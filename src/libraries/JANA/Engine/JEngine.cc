#include <JANA/Engine/JEngine.h>

void JEngine::Init() {

    m_desired_nthreads = 1;
    m_params->SetDefaultParameter("nthreads", m_desired_nthreads, "Desired number of worker threads, or 'Ncores' to use all available cores.");
    if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
        m_desired_nthreads = JCpuInfo::GetNumCpus();
    }
}

void JEngine::initialize() {
    m_processing_controller->initialize();
}


void JEngine::Run(bool wait_until_finished) {

    if(m_quitting) return;

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
        std::this_thread::sleep_for(std::chrono::milliseconds(m_ticker_interval_ms()));

        // Print status
        if( m_ticker_on ) PrintStatus();

        // Test for timeout
        if(m_timeout_on && m_processing_controller->is_timed_out()) {
            LOG_FATAL(m_logger) << "Detected timeout in worker! Stopping." << LOG_END;
            GetApplication()->SetExitCode((int) JApplication::ExitCode::Timeout);
            m_processing_controller->request_stop();
            // TODO: Timeout error is not obvious enough from UI. Maybe throw an exception instead?
            // TODO: Send USR2 signal to obtain a backtrace for timed out threads?
            break;
        }

        // Test for exception
        if (m_processing_controller->is_excepted()) {
            LOG_FATAL(m_logger) << "Detected exception in worker! Re-throwing on main thread." << LOG_END;
            GetApplication()->SetExitCode((int) JApplication::ExitCode::UnhandledException);
            m_processing_controller->request_stop();

            // We are going to throw the first exception and ignore the others.
            throw m_processing_controller->get_exceptions()[0];
        }

        // Test if user requested inspection
        if (m_inspecting) {
            ::InspectApplication(GetApplication());
            // While we are inside InspectApplication, any SIGINTs will lead to shutdown.
            // Once we exit InspectApplication, one SIGINT will pause processing and reopen InspectApplication.
            m_sigint_count = 0; 
            m_inspecting = false;
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
        GetApplication()->SetExitCode((int) JApplication::ExitCode::UnhandledException);
        // We are going to throw the first exception and ignore the others.
        throw m_processing_controller->get_exceptions()[0];
    }
}

void JEngine::Scale(int nthreads) {
    LOG_INFO(GetLogger()) << "Scaling to " << nthreads << " threads" << LOG_END;
    m_processing_controller->scale(nthreads);
}

void JEngine::RequestInspection() {
    m_inspecting = true;
    m_processing_controller->request_pause();
}

void JEngine::RequestPause() {
    m_processing_controller->request_pause();
}

void JEngine::RequestStop() {
    m_quitting = true;
    // m_skip_join = skip_join;
    // TODO: Verify that we don't need this
    m_processing_controller->request_stop();
}

void JEngine::WaitUntilPaused() {
    m_processing_controller->wait_until_paused();
}

void JEngine::WaitUntilStopped() {
    m_processing_controller->wait_until_stopped();
}

JPerfSummary JEngine::GetStatus() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - m_last_measurement) >= std::chrono::milliseconds(m_ticker_interval_ms())) {
        m_perf_summary = m_processing_controller->measure_performance();
        m_last_measurement = now;
    }
    return *m_perf_summary;
}

uint64_t JEngine::GetNThreads() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - m_last_measurement) >= std::chrono::milliseconds(m_ticker_interval_ms())) {
        m_perf_summary = m_processing_controller->measure_performance();
        m_last_measurement = now;
    }
    return m_perf_summary->thread_count;
}

uint64_t JEngine::GetNEventsProcessed() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - m_last_measurement) >= std::chrono::milliseconds(m_ticker_interval_ms())) {
        m_perf_summary = m_processing_controller->measure_performance();
        m_last_measurement = now;
    }
    return m_perf_summary->total_events_completed;
}

float JEngine::GetIntegratedRate() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - m_last_measurement) >= std::chrono::milliseconds(m_ticker_interval_ms())) {
        m_perf_summary = m_processing_controller->measure_performance();
        m_last_measurement = now;
    }
    return m_perf_summary->avg_throughput_hz;
}

float JEngine::GetInstantaneousRate() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - m_last_measurement) >= std::chrono::milliseconds(m_ticker_interval_ms())) {
        m_perf_summary = m_processing_controller->measure_performance();
        m_last_measurement = now;
    }
    return m_perf_summary->latest_throughput_hz;
}


void JEngine::PrintStatus() {
    if (m_extended_report()) {
        m_processing_controller->print_report();
    }
    else {
        std::lock_guard<std::mutex> lock(m_status_mutex);
        auto now = std::chrono::high_resolution_clock::now();
        if ((now - m_last_measurement) >= std::chrono::milliseconds(m_ticker_interval_ms())) {
            m_perf_summary = m_processing_controller->measure_performance();
            m_last_measurement = now;
        }
        LOG_INFO(m_logger) << "Status: " << m_perf_summary->total_events_completed << " events processed  "
                           << JTypeInfo::to_string_with_si_prefix(m_perf_summary->latest_throughput_hz) << "Hz ("
                           << JTypeInfo::to_string_with_si_prefix(m_perf_summary->avg_throughput_hz) << "Hz avg)" << LOG_END;
    }
}

void JEngine::HandleSigint() {
    m_sigint_count++;
    switch (m_sigint_count) {
        case 1:
            LOG_WARN(m_logger) << "Pausing..." << LOG_END;
            RequestInspection();
            break;
        case 2:
            LOG_FATAL(m_logger) << "Exiting gracefully..." << LOG_END;
            RequestStop();
            break;
        default:
            LOG_FATAL(m_logger) << "Exiting immediately." << LOG_END;
            exit(-2);
            break;
    }
}

