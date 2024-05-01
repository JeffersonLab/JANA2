
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>

#include <JANA/JEventSource.h>

#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JGlobalRootLock.h>
#include <JANA/Services/JPluginLoader.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Services/JLoggingService.h>
#include <JANA/Services/JGlobalRootLock.h>
#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Utils/JApplicationInspector.h>

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

    ProvideService(m_params);
    ProvideService(m_component_manager);
    ProvideService(m_plugin_loader);
    ProvideService(std::make_shared<JLoggingService>());
    ProvideService(std::make_shared<JGlobalRootLock>());
    ProvideService(std::make_shared<JTopologyBuilder>());
    ProvideService(std::make_shared<JEngine>());

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
    auto logging_service = m_service_locator->get<JLoggingService>();
    auto component_manager = m_service_locator->get<JComponentManager>();
    auto plugin_loader = m_service_locator->get<JPluginLoader>();
    auto topology_builder = m_service_locator->get<JTopologyBuilder>();

    // Set logger on JApplication itself
    m_logger = logging_service->get_logger("JApplication");
    m_logger.show_classname = false;

    // Attach all plugins
    plugin_loader->attach_plugins(component_manager.get());

    // Give all components a JApplication pointer and a logger
    component_manager->preinitialize_components();

    // Resolve all event sources now that all plugins have been loaded
    component_manager->resolve_event_sources();

    // Call Summarize() and Init() in order to populate JComponentSummary and JParameterManager, respectively
    component_manager->initialize_components();

    // Set desired nthreads. We parse the 'nthreads' parameter two different ways for backwards compatibility.
    m_desired_nthreads = 1;
    m_params->SetDefaultParameter("nthreads", m_desired_nthreads, "Desired number of worker threads, or 'Ncores' to use all available cores.");
    if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
        m_desired_nthreads = JCpuInfo::GetNumCpus();
    }

    m_params->SetDefaultParameter("jana:ticker_interval", m_ticker_interval_ms, "Controls the ticker interval (in ms)");
    m_params->SetDefaultParameter("jana:extended_report", m_extended_report, "Controls whether the ticker shows simple vs detailed performance metrics");


    /*
    int engine_choice = 0;
    m_params->SetDefaultParameter("jana:engine", engine_choice,
                                  "0: Use arrow engine")->SetIsAdvanced(true);

    if (engine_choice != 0) {
        LOG_WARN(m_logger) << "Unrecognized engine choice! Falling back to jana:engine=0" << LOG_END;
    }
    */
    topology_builder->create_topology();
    ProvideService(std::make_shared<JArrowProcessingController>());
    m_processing_controller = m_service_locator->get<JArrowProcessingController>();  // Get deps from SL
    m_processing_controller->initialize();

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
void JEngine::Run(bool wait_until_finished) {
    m_engine->Run(wait_until_finished);
}

void JApplication::Scale(int nthreads) {
    m_engine->Scale(nthreads);
}

void JApplication::Inspect() {
    ::InspectApplication(this);
    // While we are inside InspectApplication, any SIGINTs will lead to shutdown.
    // Once we exit InspectApplication, one SIGINT will pause processing and reopen InspectApplication.
    m_sigint_count = 0; 
    m_inspecting = false;
}

void JApplication::Stop(bool wait_until_idle) {
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
        m_processing_controller->request_stop();
        if (wait_until_idle) {
            m_processing_controller->wait_until_stopped();
        }

    }
}

void JApplication::Quit(bool skip_join) {
    m_engine->Quit(skip_join);

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

void JApplication::HandleSigint() {
    m_sigint_count++;
    switch (m_sigint_count) {
        case 1:
            LOG_WARN(m_logger) << "Entering Inspector..." << LOG_END;
            m_inspecting = true;
            m_processing_controller->request_pause();
            break;
        case 2:
            LOG_FATAL(m_logger) << "Exiting gracefully..." << LOG_END;
            japp->Quit(false);
            break;
        case 3:
            LOG_FATAL(m_logger) << "Exiting without waiting for threads to join..." << LOG_END;
            japp->Quit(true);
            break;
        default:
            LOG_FATAL(m_logger) << "Exiting immediately." << LOG_END;
            exit(-2);
    }

}

const JComponentSummary& JApplication::GetComponentSummary() {
    /// Returns a data object describing all components currently running
    return m_component_manager->get_component_summary();
}

// Performance/status monitoring
void JApplication::SetTicker(bool ticker_on) {
    m_ticker_on = ticker_on;
}

bool JApplication::IsTickerEnabled() {
    return m_ticker_on;
}

void JApplication::SetTimeoutEnabled(bool enabled) {
    m_timeout_on = enabled;
}

bool JApplication::IsTimeoutEnabled() {
    return m_timeout_on;
}

void JApplication::PrintStatus() {
    if (m_extended_report) {
        m_processing_controller->print_report();
    }
    else {
        std::lock_guard<std::mutex> lock(m_status_mutex);
        update_status();
        LOG_INFO(m_logger) << "Status: " << m_perf_summary->total_events_completed << " events processed  "
                           << JTypeInfo::to_string_with_si_prefix(m_perf_summary->latest_throughput_hz) << "Hz ("
                           << JTypeInfo::to_string_with_si_prefix(m_perf_summary->avg_throughput_hz) << "Hz avg)" << LOG_END;
    }
}

void JApplication::PrintFinalReport() {
    m_processing_controller->print_final_report();
}

/// Performs a new measurement if the time elapsed since the previous measurement exceeds some threshold
void JApplication::update_status() {
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - m_last_measurement) >= std::chrono::milliseconds(m_ticker_interval_ms) || m_perf_summary == nullptr) {
        m_perf_summary = m_processing_controller->measure_performance();
        m_last_measurement = now;
    }
}

/// Returns the number of threads currently being used.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
uint64_t JApplication::GetNThreads() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    update_status();
    return m_perf_summary->thread_count;
}

/// Returns the number of events processed since Run() was called.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
uint64_t JApplication::GetNEventsProcessed() {
    return m_engine->GetPerfSummary()->total_events_completed;
    std::lock_guard<std::mutex> lock(m_status_mutex);
    update_status();
    return m_perf_summary->total_events_completed;
}

/// Returns the total integrated throughput so far in Hz since Run() was called.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
float JApplication::GetIntegratedRate() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    update_status();
    return m_perf_summary->avg_throughput_hz;
}

/// Returns the 'instantaneous' throughput in Hz since the last perf measurement was made.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
float JApplication::GetInstantaneousRate()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);
    update_status();
    return m_perf_summary->latest_throughput_hz;
}


