
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/Engine/JExecutionEngine.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Services/JGlobalRootLock.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JPluginLoader.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Services/JWiringService.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Utils/JApplicationInspector.h>

#include <chrono>
#include <sstream>
#include <unistd.h>

JApplication *japp = nullptr;


JApplication::JApplication(JParameterManager* params) {

    // We set up some very essential services here, but note 
    // that they won't have been initialized yet. We need them now
    // so that they can passively receive components, plugin names, parameter values, etc.
    // These passive operations don't require any parameters, services, or
    // logging output, so they don't need to be initialized until later.
    // They will be fully initialized in JApplication::Initialize().
    // Only then they will be exposed to the user through the service locator.

    if (params == nullptr) {
        m_params = std::make_shared<JParameterManager>();
    }
    else {
        m_params = std::shared_ptr<JParameterManager>(params);
    }
    m_component_manager = std::make_shared<JComponentManager>();
    m_plugin_loader = std::make_shared<JPluginLoader>();
    m_service_locator = std::make_unique<JServiceLocator>();
    m_execution_engine = std::make_unique<JExecutionEngine>();

    ProvideService(m_params);
    ProvideService(m_component_manager);
    ProvideService(m_plugin_loader);
    ProvideService(m_execution_engine);
    ProvideService(std::make_shared<JGlobalRootLock>());
    ProvideService(std::make_shared<JTopologyBuilder>());
    ProvideService(std::make_shared<jana::services::JWiringService>());

}


JApplication::~JApplication() {}


// Loading plugins

void JApplication::AddPlugin(std::string plugin_name) {
    m_plugin_loader->add_plugin(plugin_name);
}

void JApplication::AddPluginPath(std::string path) {
    m_plugin_loader->add_plugin_path(path);
}


// Building a ProcessingTopology

void JApplication::Add(JEventSource* event_source) {
    /// Adds the given JEventSource to the JANA context. Ownership is passed to JComponentManager.
    m_component_manager->add(event_source);
}

void JApplication::Add(JEventSourceGenerator *source_generator) {
    /// Adds the given JEventSourceGenerator to the JANA context. Ownership is passed to JComponentManager.
    m_component_manager->add(source_generator);
}

void JApplication::Add(JFactoryGenerator *factory_generator) {
    /// Adds the given JFactoryGenerator to the JANA context. Ownership is passed to JComponentManager.
    m_component_manager->add(factory_generator);
}

void JApplication::Add(JEventProcessor* processor) {
    /// Adds the given JEventProcessor to the JANA context. Ownership is passed to JComponentManager.
    m_component_manager->add(processor);
}

void JApplication::Add(std::string event_source_name) {
    /// Adds the event source name (e.g. a file or socket name) to the JANA context. JANA will instantiate
    /// the corresponding JEventSource using a user-provided JEventSourceGenerator.
    m_component_manager->add(event_source_name);
}

void JApplication::Add(JEventUnfolder* unfolder) {
    /// Adds the given JEventUnfolder to the JANA context. Ownership is passed to JComponentManager.
    m_component_manager->add(unfolder);
}


void JApplication::Initialize() {

    /// Initialize the application in preparation for data processing.
    /// This is called by the Run method so users will usually not need to call this directly.

    // Only run this once
    if (m_initialized) return;

    // Now that all parameters, components, plugin names, etc have been set, 
    // we can expose our builtin services to the user via GetService()
    m_services_available = true;

    // We trigger initialization 
    m_service_locator->get<JParameterManager>();
    auto component_manager = m_service_locator->get<JComponentManager>();
    auto plugin_loader = m_service_locator->get<JPluginLoader>();
    auto topology_builder = m_service_locator->get<JTopologyBuilder>();

    // Set logger on JApplication itself
    m_logger = m_params->GetLogger("jana");

    if (m_logger.level > JLogger::Level::INFO) {
        std::ostringstream oss;
        oss << "Initializing..." << std::endl << std::endl;
        JVersion::PrintVersionDescription(oss);
        LOG_WARN(m_logger) << oss.str() << LOG_END;
    }
    else {
        std::ostringstream oss;
        oss << "Initializing..." << std::endl;
        JVersion::PrintSplash(oss);
        JVersion::PrintVersionDescription(oss);
        LOG_WARN(m_logger) << oss.str() << LOG_END;
    }

    // Attach all plugins
    plugin_loader->attach_plugins(component_manager.get());

    // Resolve and initialize all components
    component_manager->configure_components();

    // Set desired nthreads. We parse the 'nthreads' parameter two different ways for backwards compatibility.
    m_desired_nthreads = 1;
    m_params->SetDefaultParameter("nthreads", m_desired_nthreads, "Desired number of worker threads, or 'Ncores' to use all available cores.");
    if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
        m_desired_nthreads = JCpuInfo::GetNumCpus();
    }

    topology_builder->create_topology();
    auto execution_engine = m_service_locator->get<JExecutionEngine>();
    m_initialized = true;
    // This needs to be at the end so that m_initialized==false while InitPlugin() is being called
}

/// @brief Run the application, launching 1 or more threads to do the work.
///
/// This will initialize the application, attaching plugins etc. and launching
/// threads to process events/time slices. This will then either return immediately
/// (if wait_until_finish=false) or enter a lazy loop checking the progress
/// of the data processing (if wait_until_finish=true).
///
/// In the `wait_until_finished` mode, this will run continuously until 
/// the JProcessingController indicates it is finished or stopped or 
/// some other condition exists that would cause it to end early. Under
/// normal conditions, the data processing stops when polling JProcessingController::is_finished()
/// indicates the JArrowTopology is in the `JArrowTopology::Status::Finished`
/// state. This will occur when all event sources have been exhausted and
/// all events have been processed such that all JWorkers have stopped.
///
/// See JProcessingController::run() for more details.
///
/// @param [in] wait_until_finished If true (default) do not return until the work has completed.
void JApplication::Run(bool wait_until_stopped, bool finish) {

    Initialize();
    if(m_quitting) return;

    // At this point, all components should have been provided and all parameter values should have been set.
    // Let's report what we found!
    //
    // You might be wondering why we do this here instead of at the end of Initialize(), which might make more sense.
    // The reason is that there might be other things, specifically JBenchmarker, that do request Parameters and Services
    // but aren't JComponents (yet). These have to happen after JApplication::Initialize() and before the parameter table
    // gets printed. Because of their weird position, they are not able to add additional plugins or components, nor 
    // submit Parameter values.
    //
    // Print summary of all config parameters
    m_params->PrintParameters();

    LOG_WARN(m_logger) << "Starting processing with " << m_desired_nthreads << " threads requested..." << LOG_END;
    m_execution_engine->ScaleWorkers(m_desired_nthreads);
    m_execution_engine->RunTopology();

    if (!wait_until_stopped) {
        return;
    }

    m_execution_engine->RunSupervisor();
    if (finish) {
        m_execution_engine->FinishTopology();
    }

    // Join all threads
    if (!m_skip_join) {
        m_execution_engine->ScaleWorkers(0);
    }
}


void JApplication::Scale(int nthreads) {
    LOG_WARN(m_logger) << "Scaling to " << nthreads << " threads" << LOG_END;
    m_execution_engine->ScaleWorkers(nthreads);
    m_execution_engine->RunTopology();
}

void JApplication::Inspect() {
    ::InspectApplication(this);
    // While we are inside InspectApplication, any SIGINTs will lead to shutdown.
    // Once we exit InspectApplication, one SIGINT will pause processing and reopen InspectApplication.
    m_sigint_count = 0; 
    m_inspecting = false;
}

void JApplication::Stop(bool wait_until_stopped, bool finish) {
    if (!m_initialized) {
        // People might call Stop() during Initialize() rather than Run().
        // For instance, during JEventProcessor::Init, or via Ctrl-C.
        // If this is the case, we finish with initialization and then cancel the Run().
        // We don't wait on  because we don't want to Finalize() anything
        // we haven't Initialize()d yet.
        m_quitting = true;
    }
    else {
        // Once we've called Initialize(), we can Finish() all of our components
        // whenever we like
        m_execution_engine->DrainTopology();
        if (wait_until_stopped) {
            m_execution_engine->RunSupervisor();
            if (finish) {
                m_execution_engine->FinishTopology();
            }
        }
    }
}

void JApplication::Quit(bool skip_join) {

    if (m_initialized) {
        m_skip_join = skip_join;
        m_quitting = true;
        if (!skip_join && m_execution_engine != nullptr) {
            Stop(true);
        }
    }

    // People might call Quit() during Initialize() rather than Run().
    // For instance, during JEventProcessor::Init, or via Ctrl-C.
    // If this is the case, we exit immediately rather than make the user
    // wait on a long Initialize() if no data has been generated yet.

    _exit(m_exit_code);
}

void JApplication::SetExitCode(int exit_code) {
    /// Set a value of the exit code in that can be later retrieved
    /// using GetExitCode. This is so the executable can return
    /// a meaningful error code if processing is stopped prematurely,
    /// but the program is able to stop gracefully without a hard
    /// exit. See also GetExitCode.

    m_exit_code = exit_code;
}

int JApplication::GetExitCode() {
    /// Returns the currently set exit code. This can be used by
    /// JProcessor/JFactory classes to communicate an appropriate
    /// exit code that a jana program can return upon exit. The
    /// value can be set via the SetExitCode method.

    return m_exit_code;
}

const JComponentSummary& JApplication::GetComponentSummary() {
    /// Returns a data object describing all components currently running
    return m_component_manager->get_component_summary();
}

// Performance/status monitoring
void JApplication::SetTicker(bool ticker_on) {
    m_execution_engine->SetTickerEnabled(ticker_on);
}

bool JApplication::IsTickerEnabled() {
    return m_execution_engine->IsTickerEnabled();
}

void JApplication::SetTimeoutEnabled(bool enabled) {
    m_execution_engine->SetTimeoutEnabled(enabled);
}

bool JApplication::IsTimeoutEnabled() {
    return m_execution_engine->IsTimeoutEnabled();
}

bool JApplication::IsDrainingQueues() {
    return (m_execution_engine->GetRunStatus() == JExecutionEngine::RunStatus::Draining);
}

/// Returns the number of threads currently being used.
uint64_t JApplication::GetNThreads() {
    return m_execution_engine->GetPerf().thread_count;
}

/// Returns the number of events processed since Run() was called.
uint64_t JApplication::GetNEventsProcessed() {
    return m_execution_engine->GetPerf().event_count;
}

/// Returns the total integrated throughput so far in Hz since Run() was called.
float JApplication::GetIntegratedRate() {
    return m_execution_engine->GetPerf().throughput_hz;
}

/// Returns the 'instantaneous' throughput in Hz since the last such call was made.
float JApplication::GetInstantaneousRate()
{
    std::lock_guard<std::mutex> lock(m_inst_rate_mutex);
    auto latest_event_count = m_execution_engine->GetPerf().event_count;
    auto latest_time = JExecutionEngine::clock_t::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(latest_time - m_last_measurement_time).count();
    auto instantaneous_throughput = (duration_ms == 0) ? 0 : (latest_event_count - m_last_event_count) * 1000.0 / duration_ms;

    m_last_event_count = latest_event_count;
    m_last_measurement_time = latest_time;

    return instantaneous_throughput;
}

void JApplication::PrintStatus() {
    auto perf = m_execution_engine->GetPerf();
    LOG_INFO(m_logger) << "Topology status:     " << ToString(perf.runstatus) << LOG_END;
    LOG_INFO(m_logger) << "Worker thread count: " << perf.thread_count << LOG_END;
    LOG_INFO(m_logger) << "Events processed:    " << perf.event_count << LOG_END;
    LOG_INFO(m_logger) << "Uptime [s]:          " << perf.uptime_ms*1000 << LOG_END;
    LOG_INFO(m_logger) << "Throughput [Hz]:     " << perf.throughput_hz << LOG_END;
}



