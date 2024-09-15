
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>

#include <JANA/JEventSource.h>

#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JGlobalRootLock.h>
#include <JANA/Services/JPluginLoader.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Services/JWiringService.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Services/JGlobalRootLock.h>
#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Utils/JApplicationInspector.h>

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

    ProvideService(m_params);
    ProvideService(m_component_manager);
    ProvideService(m_plugin_loader);
    ProvideService(std::make_shared<JGlobalRootLock>());
    ProvideService(std::make_shared<JTopologyBuilder>());

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

    std::ostringstream oss;
    oss << "Initializing..." << std::endl;
    JVersion::PrintSplash(oss);
    JVersion::PrintVersionDescription(oss);
    LOG_INFO(m_logger) << oss.str() << LOG_END;

    // Now that all parameters, components, plugin names, etc have been set, 
    // we can expose our builtin services to the user via GetService()
    m_services_available = true;
    
    // We trigger initialization 
    auto component_manager = m_service_locator->get<JComponentManager>();
    auto plugin_loader = m_service_locator->get<JPluginLoader>();
    auto topology_builder = m_service_locator->get<JTopologyBuilder>();

    // Set logger on JApplication itself
    m_logger = m_params->GetLogger("jana");
    m_logger.show_classname = false;

    // Set up wiring
    ProvideService(std::make_shared<jana::services::JWiringService>());

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

    m_params->SetDefaultParameter("jana:ticker_interval", m_ticker_interval_ms, "Controls the ticker interval (in ms)");
    m_params->SetDefaultParameter("jana:extended_report", m_extended_report, "Controls whether the ticker shows simple vs detailed performance metrics");

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
void JApplication::Run(bool wait_until_finished) {

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

    LOG_INFO(m_logger) << GetComponentSummary() << LOG_END;
    LOG_INFO(m_logger) << "Starting processing with " << m_desired_nthreads << " threads requested..." << LOG_END;
    m_processing_controller->run(m_desired_nthreads);

    if (!wait_until_finished) {
        return;
    }

    // Monitor status of all threads
    while (!m_quitting) {

        // If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
        // This flag is used by the integrated rate calculator
        if (!m_draining_queues) {
            bool draining = true;
            for (auto evt_src : m_component_manager->get_evt_srces()) {
                draining &= (evt_src->GetStatus() == JEventSource::Status::Finalized);
            }
            m_draining_queues = draining;
        }

        // Run until topology is deactivated, either because it finished or because another thread called stop()
        if (m_processing_controller->is_stopped()) {
            LOG_INFO(m_logger) << "All workers have stopped." << LOG_END;
            break;
        }
        if (m_processing_controller->is_finished()) {
            LOG_INFO(m_logger) << "All workers have finished." << LOG_END;
            break;
        }

        // Sleep a few cycles
        std::this_thread::sleep_for(std::chrono::milliseconds(m_ticker_interval_ms));

        // Print status
        if( m_ticker_on ) PrintStatus();

        // Test for timeout
        if(m_timeout_on && m_processing_controller->is_timed_out()) {
            LOG_FATAL(m_logger) << "Detected timeout in worker! Stopping." << LOG_END;
            SetExitCode((int) ExitCode::Timeout);
            m_processing_controller->request_stop();
            // TODO: Timeout error is not obvious enough from UI. Maybe throw an exception instead?
            // TODO: Send USR2 signal to obtain a backtrace for timed out threads?
            break;
        }

        // Test for exception
        if (m_processing_controller->is_excepted()) {
            LOG_FATAL(m_logger) << "Detected exception in worker! Re-throwing on main thread." << LOG_END;
            SetExitCode((int) ExitCode::UnhandledException);
            m_processing_controller->request_stop();

            // We are going to throw the first exception and ignore the others.
            throw m_processing_controller->get_exceptions()[0];
        }

        if (m_inspecting) {
            Inspect();
        }
    }

    // Join all threads
    if (!m_skip_join) {
        LOG_INFO(m_logger) << "Merging threads ..." << LOG_END;
        m_processing_controller->wait_until_stopped();
    }

    LOG_INFO(m_logger) << "Event processing ended." << LOG_END;
    PrintFinalReport();

    // Test for exception one more time, in case it shut down the topology before the supervisor could detect it
    if (m_processing_controller->is_excepted()) {
        LOG_FATAL(m_logger) << "Exception in worker!" << LOG_END;
        SetExitCode((int) ExitCode::UnhandledException);
        // We are going to throw the first exception and ignore the others.
        throw m_processing_controller->get_exceptions()[0];
    }
}


void JApplication::Scale(int nthreads) {
    LOG_INFO(m_logger) << "Scaling to " << nthreads << " threads" << LOG_END;
    m_processing_controller->scale(nthreads);
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

    if (m_initialized) {
        m_skip_join = skip_join;
        m_quitting = true;
        if (!skip_join && m_processing_controller != nullptr) {
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


