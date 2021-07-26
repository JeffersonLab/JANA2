
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

JApplication *japp = nullptr;


JApplication::JApplication(JParameterManager* params) {

    if (params == nullptr) {
        _params = std::make_shared<JParameterManager>();
    }
    else {
        _params = std::shared_ptr<JParameterManager>(params);
    }

    _service_locator.provide(_params);
    _service_locator.provide(std::make_shared<JLoggingService>());
    _service_locator.provide(std::make_shared<JPluginLoader>(this));
    _service_locator.provide(std::make_shared<JComponentManager>(this));

    _plugin_loader = _service_locator.get<JPluginLoader>();
    _component_manager = _service_locator.get<JComponentManager>();

    _logger = _service_locator.get<JLoggingService>()->get_logger("JApplication");
    _logger.show_classname = false;
}


JApplication::~JApplication() {}


// Loading plugins

void JApplication::AddPlugin(std::string plugin_name) {
    _plugin_loader->add_plugin(plugin_name);
}

void JApplication::AddPluginPath(std::string path) {
    _plugin_loader->add_plugin_path(path);
}


// Building a ProcessingTopology

void JApplication::Add(JEventSource* event_source) {
    _component_manager->add(event_source);
}

void JApplication::Add(JEventSourceGenerator *source_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param source_generator pointer to source generator to add. Ownership is passed to JApplication
    _component_manager->add(source_generator);
}

void JApplication::Add(JFactoryGenerator *factory_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication
    _component_manager->add(factory_generator);
}

void JApplication::Add(JEventProcessor* processor) {
    _component_manager->add(processor);
}

void JApplication::Add(std::string event_source_name) {
    _component_manager->add(event_source_name);
}


// Controlling processing

void JApplication::Initialize() {

    /// Initialize the application in preparation for data processing.
    /// This is called by the Run method so users will usually not
    /// need to call this directly.

    try {
        // Only run this once
        if (_initialized) return;

        // Attach all plugins
        _plugin_loader->attach_plugins(_component_manager.get());

        // Set desired nthreads
        _desired_nthreads = JCpuInfo::GetNumCpus();
        _params->SetDefaultParameter("nthreads", _desired_nthreads, "The total number of worker threads");
        _params->SetDefaultParameter("jana:extended_report", _extended_report);

        _component_manager->resolve_event_sources();

        int engine_choice = 0;
        _params->SetDefaultParameter("jana:engine", engine_choice, "0: Arrow engine, 1: Debug engine");

        if (engine_choice == 0) {
            auto topology = JArrowTopology::from_components(_component_manager, _params, _desired_nthreads);
            auto japc = std::make_shared<JArrowProcessingController>(topology);
            _service_locator.provide(japc);  // Make concrete class available via SL
            _processing_controller = _service_locator.get<JArrowProcessingController>();  // Get deps from SL
            _service_locator.provide(_processing_controller);  // Make abstract class available via SL
        }
        else {
            auto jdpc = std::make_shared<JDebugProcessingController>(_component_manager.get());
            _service_locator.provide(jdpc);  // Make the concrete class available via SL
            _processing_controller = _service_locator.get<JDebugProcessingController>();  // Get deps from SL
            _service_locator.provide(_processing_controller); // Make abstract class available via SL
        }

        _processing_controller->initialize();
        _initialized = true;
    }
    catch (JException& e) {
        LOG_FATAL(_logger) << e << LOG_END;
        exit(-1);
    }
}

void JApplication::Run(bool wait_until_finished) {

    Initialize();
    if(_quitting) return;

    // Print summary of all config parameters (if any aren't default)
    _params->PrintParameters(false);

    LOG_INFO(_logger) << GetComponentSummary() << LOG_END;
    LOG_INFO(_logger) << "Starting processing ..." << LOG_END;
    _processing_controller->run(_desired_nthreads);

    if (!wait_until_finished) {
        return;
    }

    // Monitor status of all threads
    while (!_quitting) {

        // If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
        // This flag is used by the integrated rate calculator
        if (!_draining_queues) {
            bool draining = true;
            for (auto evt_src : _component_manager->get_evt_srces()) {
                draining &= (evt_src->GetStatus() == JEventSource::SourceStatus::Finished);
            }
            _draining_queues = draining;
        }

        // Run until topology is deactivated, either because it finished or because another thread called stop()
        if (_processing_controller->is_stopped() || _processing_controller->is_finished()) {
            LOG_INFO(_logger) << "All threads have ended." << LOG_END;
            break;
        }

        // Sleep a few cycles
        std::this_thread::sleep_for(_ticker_interval);

        // Print status
        if( _ticker_on ) PrintStatus();

        // Test for timeout
        if(_timeout_on && _processing_controller->is_timed_out()) {
            LOG_FATAL(_logger) << "Timeout detected." << LOG_END;
            SetExitCode(22);  // TODO: What are the exit codes, and which corresponds to timeout?
            break;
        }
    }

    // Join all threads
    if (!_skip_join) {
        LOG_INFO(_logger) << "Merging threads ..." << LOG_END;
        _processing_controller->wait_until_stopped();
    }

    LOG_INFO(_logger) << "Event processing ended." << LOG_END;
    PrintFinalReport();
}


void JApplication::Scale(int nthreads) {
    _processing_controller->scale(nthreads);
}

void JApplication::Stop(bool wait_until_idle) {
    _processing_controller->request_stop();
    if (wait_until_idle) {
        _processing_controller->wait_until_stopped();
    }
}

void JApplication::Quit(bool skip_join) {
    _skip_join = skip_join;
    _quitting = true;
    if (_processing_controller != nullptr) {
        Stop(skip_join);
    }
}

void JApplication::SetExitCode(int exit_code) {
    /// Set a value of the exit code in that can be later retrieved
    /// using GetExitCode. This is so the executable can return
    /// a meaningful error code if processing is stopped prematurely,
    /// but the program is able to stop gracefully without a hard
    /// exit. See also GetExitCode.

    _exit_code = exit_code;
}

int JApplication::GetExitCode() {
    /// Returns the currently set exit code. This can be used by
    /// JProcessor/JFactory classes to communicate an appropriate
    /// exit code that a jana program can return upon exit. The
    /// value can be set via the SetExitCode method.

    return _exit_code;
}

JComponentSummary JApplication::GetComponentSummary() {
    /// Returns a data object describing all components currently running
    return _component_manager->get_component_summary();
}

// Performance/status monitoring
void JApplication::SetTicker(bool ticker_on) {
    _ticker_on = ticker_on;
}

void JApplication::EnableTimeout(bool enabled) {
    _timeout_on = enabled;
}

void JApplication::PrintStatus() {
    if (_extended_report) {
        _processing_controller->print_report();
    }
    else {
        std::lock_guard<std::mutex> lock(_status_mutex);
        update_status();
        LOG_INFO(_logger) << "Status: " << _perf_summary->total_events_completed << " events processed  "
                          << JTypeInfo::to_string_with_si_prefix(_perf_summary->latest_throughput_hz) << "Hz ("
                          << JTypeInfo::to_string_with_si_prefix(_perf_summary->avg_throughput_hz) << "Hz avg)" << LOG_END;
    }
}

void JApplication::PrintFinalReport() {
    _processing_controller->print_final_report();
}

/// Performs a new measurement if the time elapsed since the previous measurement exceeds some threshold
void JApplication::update_status() {
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - _last_measurement) >= _ticker_interval || _perf_summary == nullptr) {
        _perf_summary = _processing_controller->measure_performance();
        _last_measurement = now;
    }
}

/// Returns the number of threads currently being used.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
uint64_t JApplication::GetNThreads() {
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->thread_count;
}

/// Returns the number of events processed since Run() was called.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
uint64_t JApplication::GetNEventsProcessed() {
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->total_events_completed;
}

/// Returns the total integrated throughput so far in Hz since Run() was called.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
float JApplication::GetIntegratedRate() {
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->avg_throughput_hz;
}

/// Returns the 'instantaneous' throughput in Hz since the last perf measurement was made.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
float JApplication::GetInstantaneousRate()
{
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->latest_throughput_hz;
}


