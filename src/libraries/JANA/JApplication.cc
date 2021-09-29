
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>

#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JPluginLoader.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Engine/JDebugProcessingController.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Utils/JAutoActivator.h>

JApplication *japp = nullptr;


JApplication::JApplication(JParameterManager* params) {

    if (params == nullptr) {
        m_params = std::make_shared<JParameterManager>();
    }
    else {
        m_params = std::shared_ptr<JParameterManager>(params);
    }

    m_service_locator.provide(m_params);
    m_service_locator.provide(std::make_shared<JLoggingService>());
    m_service_locator.provide(std::make_shared<JPluginLoader>(this));
    m_service_locator.provide(std::make_shared<JComponentManager>(this));

    m_plugin_loader = m_service_locator.get<JPluginLoader>();
    m_component_manager = m_service_locator.get<JComponentManager>();

    m_logger = m_service_locator.get<JLoggingService>()->get_logger("JApplication");
    m_logger.show_classname = false;
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
    m_component_manager->add(event_source);
}

void JApplication::Add(JEventSourceGenerator *source_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param source_generator pointer to source generator to add. Ownership is passed to JApplication
    m_component_manager->add(source_generator);
}

void JApplication::Add(JFactoryGenerator *factory_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication
    m_component_manager->add(factory_generator);
}

void JApplication::Add(JEventProcessor* processor) {
    m_component_manager->add(processor);
}

void JApplication::Add(std::string event_source_name) {
    m_component_manager->add(event_source_name);
}


// Controlling processing

void JApplication::Initialize() {

    /// Initialize the application in preparation for data processing.
    /// This is called by the Run method so users will usually not
    /// need to call this directly.

    try {
        // Only run this once
        if (m_initialized) return;

        // Attach all plugins
        m_plugin_loader->attach_plugins(m_component_manager.get());

        // Look for factories to auto-activate
        if (JAutoActivator::IsRequested(m_params)) {
            m_component_manager->add(new JAutoActivator);
        }

        // Set desired nthreads. We parse the 'nthreads' parameter two different ways for backwards compatibility.
        m_desired_nthreads = 1;
            m_params->SetDefaultParameter("nthreads", m_desired_nthreads, "The total number of worker threads");
            if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
            m_desired_nthreads = JCpuInfo::GetNumCpus();
        }

        m_params->SetDefaultParameter("jana:extended_report", m_extended_report);

        m_component_manager->resolve_event_sources();

        int engine_choice = 0;
        m_params->SetDefaultParameter("jana:engine", engine_choice, "0: Arrow engine, 1: Debug engine");

        if (engine_choice == 0) {
            auto topology = JArrowTopology::from_components(m_component_manager, m_params, m_desired_nthreads);
            auto japc = std::make_shared<JArrowProcessingController>(topology);
            m_service_locator.provide(japc);  // Make concrete class available via SL
            m_processing_controller = m_service_locator.get<JArrowProcessingController>();  // Get deps from SL
            m_service_locator.provide(m_processing_controller);  // Make abstract class available via SL
        }
        else {
            auto jdpc = std::make_shared<JDebugProcessingController>(m_component_manager.get());
            m_service_locator.provide(jdpc);  // Make the concrete class available via SL
            m_processing_controller = m_service_locator.get<JDebugProcessingController>();  // Get deps from SL
            m_service_locator.provide(m_processing_controller); // Make abstract class available via SL
        }

        m_processing_controller->initialize();
        m_initialized = true;
    }
    catch (JException& e) {
        LOG_FATAL(m_logger) << e << LOG_END;
        exit(-1);
    }
}

void JApplication::Run(bool wait_until_finished) {

    Initialize();
    if(m_quitting) return;

    // Print summary of all config parameters (if any aren't default)
    m_params->PrintParameters(false);

    LOG_INFO(m_logger) << GetComponentSummary() << LOG_END;
    LOG_INFO(m_logger) << "Starting processing ..." << LOG_END;
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
                draining &= (evt_src->GetStatus() == JEventSource::SourceStatus::Finished);
            }
            m_draining_queues = draining;
        }

        // Run until topology is deactivated, either because it finished or because another thread called stop()
        if (m_processing_controller->is_stopped() || m_processing_controller->is_finished()) {
            LOG_INFO(m_logger) << "All threads have ended." << LOG_END;
            break;
        }

        // Sleep a few cycles
        std::this_thread::sleep_for(m_ticker_interval);

        // Print status
        if( m_ticker_on ) PrintStatus();

        // Test for timeout
        if(m_timeout_on && m_processing_controller->is_timed_out()) {
            LOG_FATAL(m_logger) << "Timeout detected." << LOG_END;
            SetExitCode(22);  // TODO: What are the exit codes, and which corresponds to timeout?
            break;
        }
    }

    // Join all threads
    if (!m_skip_join) {
        LOG_INFO(m_logger) << "Merging threads ..." << LOG_END;
        m_processing_controller->wait_until_stopped();
    }

    LOG_INFO(m_logger) << "Event processing ended." << LOG_END;
    PrintFinalReport();
}


void JApplication::Scale(int nthreads) {
    m_processing_controller->scale(nthreads);
}

void JApplication::Stop(bool wait_until_idle) {
    m_processing_controller->request_stop();
    if (wait_until_idle) {
        m_processing_controller->wait_until_stopped();
    }
}

void JApplication::Quit(bool skip_join) {
    m_skip_join = skip_join;
    m_quitting = true;
    if (m_processing_controller != nullptr) {
        Stop(skip_join);
    }
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

JComponentSummary JApplication::GetComponentSummary() {
    /// Returns a data object describing all components currently running
    return m_component_manager->get_component_summary();
}

// Performance/status monitoring
void JApplication::SetTicker(bool ticker_on) {
    m_ticker_on = ticker_on;
}

void JApplication::EnableTimeout(bool enabled) {
    m_timeout_on = enabled;
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
    if ((now - m_last_measurement) >= m_ticker_interval || m_perf_summary == nullptr) {
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


