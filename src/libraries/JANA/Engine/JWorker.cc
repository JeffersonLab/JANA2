
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Engine/JWorker.h>
#include <JANA/Utils/JCpuInfo.h>

/// This allows someone (aka JArrowProcessingController) to declare that this
/// thread has timed out. This ensures that the underlying thread will be detached
/// rather than joined when it is time to wait_for_stop().
/// Note: Another option was to non-cooperatively kill the thread, at the risk of
/// resource leaks and data corruption. This is NOT supported by std::thread, because
/// smarter people than me have concluded it is a Bad Idea, so we'd have to use the
/// underlying pthread interface if we decide we REALLY want this feature. It doesn't
/// make a difference in the common case because the program will terminate upon timeout
/// anyhow; it only matters in case we are running JANA from a REPL. My instinct is to
/// leave the offending thread alone so that the user has a chance to attach a debugger
/// to the process.
void JWorker::declare_timeout() {
    m_run_state = RunState::TimedOut;
}

void JWorker::measure_perf(WorkerSummary& summary) {
    // Read (do not clear) worker metrics
    // Read and clear arrow metrics
    // Push arrow metrics upstream

    JArrowMetrics latest_arrow_metrics;
    latest_arrow_metrics.clear();
    latest_arrow_metrics.take(m_arrow_metrics); // move local arrow metrics onto stack
    std::string arrow_name = "idle";
    {
        std::lock_guard<std::mutex> lock(m_assignment_mutex);
        if (m_assignment != nullptr) {
            m_assignment->get_metrics().update(latest_arrow_metrics); // propagate to global arrow context
            arrow_name = m_assignment->get_name();
        }
    }
    // Unpack latest_arrow_metrics, add to WorkerSummary

    // If we wanted to average over our measurement interval only, we would also do:
    // From inside JProcessingController, do _after_ JWorker::measure_perf() for all workers:
    // for all arrows:
    //     arrow->overall_metrics.take(arrow->interval_metrics);

    // From right here, at the very end:
    //     worker->overall_metrics.take(worker->interval_metrics);

    JWorkerMetrics latest_worker_metrics;
    latest_worker_metrics.clear();
    latest_worker_metrics.update(m_worker_metrics); // nondestructive
    // Unpack latest_worker_metrics, add to WorkerSummary

    using millis = std::chrono::duration<double, std::milli>;

    long scheduler_visit_count;
    JWorkerMetrics::duration_t total_useful_time, total_retry_time, total_scheduler_time, total_idle_time;
    JWorkerMetrics::duration_t last_useful_time, last_retry_time, last_scheduler_time, last_idle_time;
    JWorkerMetrics::time_point_t last_heartbeat;

    latest_worker_metrics.get(last_heartbeat, scheduler_visit_count, total_useful_time, total_retry_time, total_scheduler_time,
             total_idle_time, last_useful_time, last_retry_time, last_scheduler_time, last_idle_time);

    summary.total_useful_time_ms = millis(total_useful_time).count();
    summary.total_retry_time_ms = millis(total_retry_time).count();
    summary.total_scheduler_time_ms = millis(total_scheduler_time).count();
    summary.total_idle_time_ms = millis(total_idle_time).count();
    summary.last_useful_time_ms = millis(last_useful_time).count();
    summary.last_retry_time_ms = millis(last_retry_time).count();
    summary.last_scheduler_time_ms = millis(last_scheduler_time).count();
    summary.last_idle_time_ms = millis(last_idle_time).count();
    summary.last_heartbeat_ms = millis(JWorkerMetrics::clock_t::now() - last_heartbeat).count();

    summary.worker_id = m_worker_id;
    summary.cpu_id = m_cpu_id;
    summary.is_pinned = m_pin_to_cpu;
    summary.scheduler_visit_count = scheduler_visit_count;

    summary.last_arrow_name = arrow_name;

    JArrowMetrics::Status last_status;
    size_t total_message_count, last_message_count, total_queue_visits, last_queue_visits;
    JArrowMetrics::duration_t total_latency, last_latency, total_queue_latency, last_queue_latency;

    latest_arrow_metrics.get(last_status, total_message_count, last_message_count, total_queue_visits,
                             last_queue_visits, total_latency, last_latency, total_queue_latency, last_queue_latency);


    summary.last_arrow_last_latency_ms = millis(last_latency).count();
    summary.last_arrow_queue_visit_count = total_queue_visits; // TODO: Why do we have last_queue_visits?

    if (total_message_count == 0) {
        summary.last_arrow_avg_latency_ms = std::numeric_limits<double>::infinity();
        summary.last_arrow_avg_queue_latency_ms = std::numeric_limits<double>::infinity();
    }
    else {
        summary.last_arrow_avg_queue_latency_ms = millis(total_queue_latency).count() / total_message_count;
        summary.last_arrow_avg_latency_ms = millis(total_latency).count() / total_message_count;
    }
    summary.last_arrow_last_queue_latency_ms = millis(last_queue_latency).count();

    // TODO: Do we even want last_message_count? This is in [0, chunksize], nothing to do with
    //       measurement interval. total gives us the measurement interval here.
    // What about Arrow::metrics::last_message_count? This would have to be the same...

}


JWorker::JWorker(JScheduler* scheduler, unsigned worker_id, unsigned cpu_id, unsigned location_id, bool pin_to_cpu) :

        m_scheduler(scheduler),
        m_worker_id(worker_id),
        m_cpu_id(cpu_id),
        m_location_id(location_id),
        m_pin_to_cpu(pin_to_cpu),
        m_run_state(RunState::Stopped),
        m_assignment(nullptr),
        m_thread(nullptr) {

    m_arrow_metrics.clear();
    m_worker_metrics.clear();
}

JWorker::~JWorker() {
    wait_for_stop();
}

void JWorker::start() {
    if (m_run_state == RunState::Stopped) {

        m_run_state = RunState::Running;
        m_thread = new std::thread(&JWorker::loop, this);

        if (m_pin_to_cpu) {
            JCpuInfo::PinThreadToCpu(m_thread, m_cpu_id);
        }
    }
}

void JWorker::request_stop() {
    if (m_run_state == RunState::Running) {
        m_run_state = RunState::Stopping;
    }
}

void JWorker::wait_for_stop() {
    if (m_run_state == RunState::Running) {
        m_run_state = RunState::Stopping;
    }
    if (m_thread != nullptr) {
        if (m_run_state == RunState::TimedOut) {
            m_thread->detach();
            // Thread has timed out. Rather than non-cooperatively killing it,
            // we relinquish ownership of it but remember that it was ours once and
            // is still out there, somewhere, biding its time
        }
        else {
            m_thread->join();
        }
        delete m_thread;
        m_thread = nullptr;
        if (m_run_state == RunState::Stopping) {
            m_run_state = RunState::Stopped;
            // By this point, the JWorker must either be Stopped or TimedOut
        }
    }
}

void JWorker::loop() {
    using jclock_t = JWorkerMetrics::clock_t;
    try {
        LOG_DEBUG(logger) << "Worker " << m_worker_id << " has fired up." << LOG_END;
        JArrowMetrics::Status last_result = JArrowMetrics::Status::NotRunYet;

        while (m_run_state == RunState::Running) {

            LOG_DEBUG(logger) << "Worker " << m_worker_id << " is checking in" << LOG_END;
            auto start_time = jclock_t::now();

            {
                std::lock_guard<std::mutex> lock(m_assignment_mutex);
                m_assignment = m_scheduler->next_assignment(m_worker_id, m_assignment, last_result);
            }
            last_result = JArrowMetrics::Status::NotRunYet;

            auto scheduler_time = jclock_t::now();

            auto scheduler_duration = scheduler_time - start_time;
            auto idle_duration = jclock_t::duration::zero();
            auto retry_duration = jclock_t::duration::zero();
            auto useful_duration = jclock_t::duration::zero();

            if (m_assignment == nullptr) {

                LOG_DEBUG(logger) << "Worker " << m_worker_id << " idling due to lack of assignments" << LOG_END;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                idle_duration = jclock_t::now() - scheduler_time;
            }
            else {

                auto initial_backoff_time = m_assignment->get_initial_backoff_time();
                auto backoff_strategy = m_assignment->get_backoff_strategy();
                auto backoff_tries = m_assignment->get_backoff_tries();
                auto checkin_time = m_assignment->get_checkin_time();

                uint32_t current_tries = 0;
                auto backoff_duration = initial_backoff_time;

                while (current_tries <= backoff_tries &&
                       (last_result == JArrowMetrics::Status::KeepGoing || last_result == JArrowMetrics::Status::ComeBackLater || last_result == JArrowMetrics::Status::NotRunYet) &&
                       (m_run_state == RunState::Running) &&
                       (jclock_t::now() - start_time) < checkin_time) {

                    LOG_TRACE(logger) << "Worker " << m_worker_id << " is executing "
                                      << m_assignment->get_name() << LOG_END;
                    auto before_execute_time = jclock_t::now();
                    m_assignment->execute(m_arrow_metrics, m_location_id);
                    last_result = m_arrow_metrics.get_last_status();
                    useful_duration += (jclock_t::now() - before_execute_time);


                    if (last_result == JArrowMetrics::Status::KeepGoing) {
                        LOG_DEBUG(logger) << "Worker " << m_worker_id << " succeeded at "
                                          << m_assignment->get_name() << LOG_END;
                        current_tries = 0;
                        backoff_duration = initial_backoff_time;
                    }
                    else {
                        current_tries++;
                        if (backoff_tries > 0) {
                            if (backoff_strategy == JArrow::BackoffStrategy::Linear) {
                                backoff_duration += initial_backoff_time;
                            }
                            else if (backoff_strategy == JArrow::BackoffStrategy::Exponential) {
                                backoff_duration *= 2;
                            }
                            LOG_TRACE(logger) << "Worker " << m_worker_id << " backing off with "
                                              << m_assignment->get_name() << ", tries = " << current_tries
                                              << LOG_END;

                            std::this_thread::sleep_for(backoff_duration);
                            retry_duration += backoff_duration;
                        }
                    }
                }
            }
            m_worker_metrics.update(start_time, 1, useful_duration, retry_duration, scheduler_duration, idle_duration);
            if (m_assignment != nullptr) {
                JArrowMetrics latest_arrow_metrics;
                latest_arrow_metrics.clear();
                latest_arrow_metrics.take(m_arrow_metrics); // move local arrow metrics onto stack
                m_assignment->get_metrics().update(latest_arrow_metrics); // propagate to global arrow context
            }
        }

        m_scheduler->last_assignment(m_worker_id, m_assignment, last_result);
        m_assignment = nullptr; // Worker has 'handed in' the assignment
        // TODO: Make m_assignment unique_ptr?

        LOG_DEBUG(logger) << "Worker " << m_worker_id << " is exiting." << LOG_END;
    }
    catch (const JException& e) {
        // For now the excepting Worker prints the error, and then terminates the whole program.
        // Eventually we want to unify error handling across JApplication::Run, and maybe even across the entire JApplication.
        // This means that Workers must pass JExceptions back to the master thread.
        LOG_FATAL(logger) << e << LOG_END;
        exit(-1);
    }catch (std::runtime_error& e){
        // same as above
        LOG_FATAL(logger) << e.what() << LOG_END;
        exit(-1);
    }
}






