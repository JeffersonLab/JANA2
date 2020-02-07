//
// Created by nbrei on 4/4/19.
//

#include <JANA/Engine/JWorker.h>
#include <JANA/Utils/JCpuInfo.h>


void JWorker::measure_perf(WorkerSummary& summary) {
    // Read (do not clear) worker metrics
    // Read and clear arrow metrics
    // Push arrow metrics upstream

    JArrowMetrics latest_arrow_metrics;
    latest_arrow_metrics.clear();
    latest_arrow_metrics.take(_arrow_metrics); // move local arrow metrics onto stack
    if (_assignment != nullptr) {
        _assignment->get_metrics().update(latest_arrow_metrics); // propagate to global arrow context
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
    latest_worker_metrics.update(_worker_metrics); // nondestructive
    // Unpack latest_worker_metrics, add to WorkerSummary

    using millis = std::chrono::duration<double, std::milli>;

    long scheduler_visit_count;
    duration_t total_useful_time, total_retry_time, total_scheduler_time, total_idle_time;
    duration_t last_useful_time, last_retry_time, last_scheduler_time, last_idle_time;

    latest_worker_metrics.get(scheduler_visit_count, total_useful_time, total_retry_time, total_scheduler_time,
             total_idle_time, last_useful_time, last_retry_time, last_scheduler_time, last_idle_time);

    summary.total_useful_time_ms = millis(total_useful_time).count();
    summary.total_retry_time_ms = millis(total_retry_time).count();
    summary.total_scheduler_time_ms = millis(total_scheduler_time).count();
    summary.total_idle_time_ms = millis(total_idle_time).count();
    summary.last_useful_time_ms = millis(last_useful_time).count();
    summary.last_retry_time_ms = millis(last_retry_time).count();
    summary.last_scheduler_time_ms = millis(last_scheduler_time).count();
    summary.last_idle_time_ms = millis(last_idle_time).count();

    summary.worker_id = _worker_id;
    summary.cpu_id = _cpu_id;
    summary.is_pinned = _pin_to_cpu;
    summary.scheduler_visit_count = scheduler_visit_count;

    summary.last_arrow_name = ((_assignment == nullptr) ? "idle" : _assignment->get_name());

    JArrowMetrics::Status last_status;
    size_t total_message_count, last_message_count, total_queue_visits, last_queue_visits;
    duration_t total_latency, last_latency, total_queue_latency, last_queue_latency;

    latest_arrow_metrics.get(last_status, total_message_count, last_message_count, total_queue_visits,
                             last_queue_visits, total_latency, last_latency, total_queue_latency, last_queue_latency);


    summary.last_arrow_avg_latency_ms = millis(total_latency).count() / total_message_count;
    summary.last_arrow_last_latency_ms = millis(last_latency).count();
    summary.last_arrow_queue_visit_count = total_queue_visits; // TODO: Why do we have last_queue_visits?
    summary.last_arrow_avg_queue_latency_ms = millis(total_queue_latency).count() / total_message_count;
    summary.last_arrow_last_queue_latency_ms = millis(last_queue_latency).count();

    // TODO: Do we even want last_message_count? This is in [0, chunksize], nothing to do with
    //       measurement interval. total gives us the measurement interval here.
    // What about Arrow::metrics::last_message_count? This would have to be the same...

}


JWorker::JWorker(JScheduler* scheduler, unsigned worker_id, unsigned cpu_id, unsigned location_id, bool pin_to_cpu) :

        _scheduler(scheduler),
        _worker_id(worker_id),
        _cpu_id(cpu_id),
        _location_id(location_id),
        _pin_to_cpu(pin_to_cpu),
        _run_state(RunState::Stopped),
        _assignment(nullptr),
        _thread(nullptr) {

    _arrow_metrics.clear();
    _worker_metrics.clear();
}

JWorker::~JWorker() {
    wait_for_stop();
}

void JWorker::start() {
    if (_run_state == RunState::Stopped) {

        _run_state = RunState::Running;
        _thread = new std::thread(&JWorker::loop, this);

        if (_pin_to_cpu) {
            JCpuInfo::PinThreadToCpu(_thread, _cpu_id);
        }
    }
}

void JWorker::request_stop() {
    if (_run_state == RunState::Running) {
        _run_state = RunState::Stopping;
    }
}

void JWorker::wait_for_stop() {
    if (_run_state == RunState::Running) {
        _run_state = RunState::Stopping;
    }
    if (_thread != nullptr) {
        _thread->join();
        delete _thread;
        _thread = nullptr;
        _run_state = RunState::Stopped;
    }
}

void JWorker::loop() {

    try {
        LOG_DEBUG(logger) << "Worker " << _worker_id << " has fired up." << LOG_END;
        JArrowMetrics::Status last_result = JArrowMetrics::Status::NotRunYet;

        while (_run_state == RunState::Running) {

            LOG_DEBUG(logger) << "Worker " << _worker_id << " is checking in" << LOG_END;
            auto start_time = jclock_t::now();

            _assignment = _scheduler->next_assignment(_worker_id, _assignment, last_result);
            last_result = JArrowMetrics::Status::NotRunYet;

            auto scheduler_time = jclock_t::now();

            auto scheduler_duration = scheduler_time - start_time;
            auto idle_duration = duration_t::zero();
            auto retry_duration = duration_t::zero();
            auto useful_duration = duration_t::zero();

            if (_assignment == nullptr) {

                LOG_DEBUG(logger) << "Worker " << _worker_id << " idling due to lack of assignments" << LOG_END;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                idle_duration = jclock_t::now() - scheduler_time;
            }
            else {

                auto initial_backoff_time = _assignment->get_initial_backoff_time();
                auto backoff_strategy = _assignment->get_backoff_strategy();
                auto backoff_tries = _assignment->get_backoff_tries();
                auto checkin_time = _assignment->get_checkin_time();

                uint32_t current_tries = 0;
                auto backoff_duration = initial_backoff_time;

                while (current_tries <= backoff_tries &&
                       (last_result == JArrowMetrics::Status::KeepGoing || last_result == JArrowMetrics::Status::ComeBackLater || last_result == JArrowMetrics::Status::NotRunYet) &&
                       (_run_state == RunState::Running) &&
                       (jclock_t::now() - start_time) < checkin_time) {

                    LOG_TRACE(logger) << "Worker " << _worker_id << " is executing "
                                       << _assignment->get_name() << LOG_END;
                    auto before_execute_time = jclock_t::now();
                    _assignment->execute(_arrow_metrics, _location_id);
                    last_result = _arrow_metrics.get_last_status();
                    useful_duration += (jclock_t::now() - before_execute_time);


                    if (last_result == JArrowMetrics::Status::KeepGoing) {
                        LOG_DEBUG(logger) << "Worker " << _worker_id << " succeeded at "
                                           << _assignment->get_name() << LOG_END;
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
                            LOG_TRACE(logger) << "Worker " << _worker_id << " backing off with "
                                              << _assignment->get_name() << ", tries = " << current_tries
                                              << LOG_END;

                            std::this_thread::sleep_for(backoff_duration);
                            retry_duration += backoff_duration;
                        }
                    }
                }
            }
            _worker_metrics.update(1, useful_duration, retry_duration, scheduler_duration, idle_duration);
            publish_arrow_metrics();
        }

        _scheduler->last_assignment(_worker_id, _assignment, last_result);
        _assignment = nullptr; // Worker has 'handed in' the assignment
        // TODO: Make _assignment unique_ptr?

        LOG_DEBUG(logger) << "Worker " << _worker_id << " is exiting." << LOG_END;
    }
    catch (const JException& e) {
        // For now the excepting Worker prints the error, and then terminates the whole program.
        // Eventually we want to unify error handling across JApplication::Run, and maybe even across the entire JApplication.
        // This means that Workers must pass JExceptions back to the master thread.
        LOG_FATAL(logger) << e << LOG_END;
        exit(-1);
    }
}

void JWorker::publish_arrow_metrics() {
    if (_assignment != nullptr) {
        JArrowMetrics latest_arrow_metrics;
        latest_arrow_metrics.clear();
        latest_arrow_metrics.take(_arrow_metrics); // move local arrow metrics onto stack
        _assignment->get_metrics().update(latest_arrow_metrics); // propagate to global arrow context
    }
}





