
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JFactorySet.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

#include <JANA/Engine/JDebugProcessingController.h>

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

JDebugProcessingController::JDebugProcessingController(JComponentManager* jcm) : m_component_manager(jcm) {
    assert(jcm != nullptr);
}

void JDebugProcessingController::acquire_services(JServiceLocator * sl) {
    m_logger = sl->get<JLoggingService>()->get_logger("JDebugProcessingController");
}


void JDebugProcessingController::run_worker() {

    try {
        LOG_INFO(m_logger) << "Starting worker" << LOG_END;
        auto evt_srces = m_component_manager->get_evt_srces();
        auto evt_procs = m_component_manager->get_evt_procs();

        for (JEventProcessor *proc: evt_procs) {
            proc->DoInitialize();
        }

        // We don't need a JEventPool because our worker can just keep recycling the same one
        // This also makes locality trivial
        auto event = std::make_shared<JEvent>();

        for (JEventSource *evt_src: evt_srces) {

            if (m_stop_requested) continue;
            evt_src->DoInitialize();
            event->SetJEventSource(evt_src);
            event->SetJApplication(evt_src->GetApplication());

            m_component_manager->configure_event(*event);
            // auto factory_set = new JFactorySet(evt_src->GetFactoryGenerator(), m_component_manager->get_fac_gens());
            // event->SetFactorySet(factory_set);

            for (auto result = JEventSource::ReturnStatus::TryAgain;
                 result != JEventSource::ReturnStatus::Finished && !m_stop_requested;) {

                result = evt_src->DoNext(event);
                if (result == JEventSource::ReturnStatus::Success) {
                    event->SetEventNumber(++m_total_events_emitted);

                    for (JEventProcessor *proc: evt_procs) {
                        proc->DoMap(event);
                        proc->DoReduce(event);
                    }
                    m_total_events_processed += 1;
                    event->GetFactorySet()->Release();
                }
            }
        }

        LOG_INFO(m_logger) << "Worker is shutting down..." << LOG_END;
        m_total_active_workers -= 1;

        if (m_total_active_workers == 0) {

            m_stop_achieved = true;
            m_finish_achieved = true;
            for (auto evt_src: evt_srces) {
                m_finish_achieved &= (evt_src->GetStatus() == JEventSource::SourceStatus::Finished);
            }

            // Previously, we finalized event processors _only_ if finish was achieved.
            // Now, however, "stop" means finalize everything, and the alternative is named "pause".
            // TODO: Update JDebugProcessingController and JProcessingController to support pause. Maybe even JApplication.
            LOG_INFO(m_logger) << "Last worker is finalizing the event processors" << LOG_END;
            for (JEventProcessor *evt_prc: evt_procs) {
                evt_prc->DoFinalize();
            }

            LOG_INFO(m_logger) << "Last worker is stopping the stopwatch" << LOG_END;
            m_perf_metrics.stop(m_total_events_processed);
        }
    }
    catch (JException& e) {
        // An exception in a worker (for any reason) triggers a
        std::lock_guard<std::mutex> lock(m_exception_mutex);
        m_exceptions.push_back(e);
        m_stop_requested = true;
    }
}


void JDebugProcessingController::initialize() {
    LOG_INFO(m_logger) << "Initializing JDebugProcessingController" << LOG_END;
}

void JDebugProcessingController::run(size_t nthreads) {
    m_stop_requested = false;
    m_stop_achieved = false;
    m_finish_achieved = false;
    m_total_active_workers = nthreads;
    m_perf_metrics.start(m_total_events_processed, nthreads);
    for (size_t i=0; i<nthreads; ++i) {
        m_workers.push_back(new std::thread(&JDebugProcessingController::run_worker, this));
    }
}

void JDebugProcessingController::scale(size_t nthreads) {
    wait_until_stopped();
    m_perf_metrics.stop(m_total_events_processed);
    m_perf_metrics.reset();
    run(nthreads);
}

void JDebugProcessingController::request_stop() {
    m_stop_requested = true;
}

void JDebugProcessingController::wait_until_stopped() {
    m_stop_requested = true;
    for (auto * worker : m_workers) {
        worker->join();
    }
    m_perf_metrics.stop(m_total_events_processed);
    for (auto * worker : m_workers) {
        delete worker;
    }
    m_workers.clear();
    m_stop_achieved = true;
}

bool JDebugProcessingController::is_stopped() {
    return m_stop_achieved;
}

bool JDebugProcessingController::is_finished() {
    return m_finish_achieved;
}

bool JDebugProcessingController::is_timed_out() {
    return false;
}

bool JDebugProcessingController::is_excepted() {
    return m_exceptions.size() > 0;
}

JDebugProcessingController::~JDebugProcessingController() {
    for (auto * worker : m_workers) {
        worker->join();
        delete worker;
    }
}

void JDebugProcessingController::print_report() {
    auto ps = measure_performance();
    std::cout << *ps << std::endl;
}

void JDebugProcessingController::print_final_report() {
    print_report();
}

std::unique_ptr<const JPerfSummary> JDebugProcessingController::measure_performance() {

    m_perf_metrics.split(m_total_events_processed);
    auto ps = std::unique_ptr<JPerfSummary>(new JPerfSummary());
    m_perf_metrics.summarize(*ps);
    return ps;
}

std::vector<JException> JDebugProcessingController::get_exceptions() const {
    return m_exceptions;
}


