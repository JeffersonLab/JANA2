#include <JANA/Engine/JEngine.h>

void JEngine::Init() {

    m_desired_nthreads = 1;
    m_params->SetDefaultParameter("nthreads", m_desired_nthreads, "Desired number of worker threads, or 'Ncores' to use all available cores.");
    if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
        m_desired_nthreads = JCpuInfo::GetNumCpus();
    }

    m_params->SetDefaultParameter("jana:ticker_interval", m_ticker_interval_ms, "Controls the ticker interval (in ms)");
    m_params->SetDefaultParameter("jana:extended_report", m_extended_report, "Controls whether the ticker shows simple vs detailed performance metrics");

}

void JEngine::Run(bool wait_until_finished) {

    Initialize();
    if(m_quitting) return;

    // Print summary of all config parameters (if any aren't default)
    m_params->PrintParameters(false);

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

void JEngine::Scale(int nthreads) {
    LOG_INFO(GetLogger()) << "Scaling to " << nthreads << " threads" << LOG_END;
    m_processing_controller->scale(nthreads);
}


void JEngine::Quit(bool skip_join) {

    if (m_initialized) {
        m_skip_join = skip_join;
        m_quitting = true;
        if (!skip_join && m_processing_controller != nullptr) {
            Stop(true);
        }
    }
}
