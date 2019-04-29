//
// Created by nbrei on 4/4/19.
//

#include <JANA/JWorker.h>


void JWorker::measure_perf(JMetrics::WorkerSummary& summary) {
    // Read (do not clear) worker metrics
    // Read and clear arrow metrics
    // Push arrow metrics upstream

    JArrowMetrics latest_arrow_metrics;
    latest_arrow_metrics.clear();
    latest_arrow_metrics.take(_arrow_metrics); // destructive
    publish_arrow_metrics();
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


JWorker::JWorker(uint32_t id, JScheduler* scheduler) :
        _scheduler(scheduler), _worker_id(id), _cpu_id(0), _pin_to_cpu(false), _run_state(RunState::Stopped),
        _assignment(nullptr), _thread(nullptr) {

    _logger = JLoggingService::logger("JWorker");
}


JWorker::JWorker(uint32_t id, uint32_t cpuid, JScheduler* scheduler) :
        _scheduler(scheduler), _worker_id(id), _cpu_id(cpuid), _pin_to_cpu(true), _run_state(RunState::Stopped),
        _assignment(nullptr), _thread(nullptr) {

    _logger = JLoggingService::logger("JWorker");
}

JWorker::~JWorker() {
    wait_for_stop();
}

void JWorker::start() {
    if (_run_state == RunState::Stopped) {
        _run_state = RunState::Running;
        _thread = new std::thread(&JWorker::loop, this);
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

    LOG_DEBUG(_logger) << "JWorker " << _worker_id << " has entered loop()." << LOG_END;
    JArrowMetrics::Status last_result = JArrowMetrics::Status::NotRunYet;

    while (_run_state == RunState::Running) {

        LOG_DEBUG(_logger) << "JWorker " << _worker_id << " is checking in" << LOG_END;
        auto start_time = jclock_t::now();

        _assignment = _scheduler->next_assignment(_worker_id, _assignment, last_result);
        last_result = JArrowMetrics::Status::NotRunYet;

        auto scheduler_time = jclock_t::now();

        auto scheduler_duration = scheduler_time - start_time;
        auto idle_duration = duration_t::zero();
        auto retry_duration = duration_t::zero();
        auto useful_duration = duration_t::zero();

        if (_assignment == nullptr) {
            LOG_DEBUG(_logger) << "JWorker " << _worker_id
                               << " idling due to lack of assignments" << LOG_END;
            std::this_thread::sleep_for(checkin_time);
            idle_duration = jclock_t::now() - scheduler_time;
        }
        else {

            uint32_t current_tries = 0;
            auto backoff_duration = initial_backoff_time;

            while (current_tries <= backoff_tries &&
                   (last_result == JArrowMetrics::Status::KeepGoing || last_result == JArrowMetrics::Status::ComeBackLater || last_result == JArrowMetrics::Status::NotRunYet) &&
                   (_run_state == RunState::Running) &&
                   (jclock_t::now() - start_time) < checkin_time) {

                LOG_DEBUG(_logger) << "JWorker " << _worker_id << " is executing "
                                   << _assignment->get_name() << LOG_END;
                auto before_execute_time = jclock_t::now();
                _assignment->execute(_arrow_metrics);
                last_result = _arrow_metrics.get_last_status();
                useful_duration += (jclock_t::now() - before_execute_time);


                if (last_result == JArrowMetrics::Status::KeepGoing) {
                    LOG_DEBUG(_logger) << "JWorker " << _worker_id << " succeeded at "
                                       << _assignment->get_name() << LOG_END;
                    current_tries = 0;
                    backoff_duration = initial_backoff_time;
                }
                else {
                    current_tries++;
                    if (backoff_tries > 0) {
                        if (backoff_strategy == BackoffStrategy::Linear) {
                            backoff_duration += initial_backoff_time;
                        }
                        else if (backoff_strategy == BackoffStrategy::Exponential) {
                            backoff_duration *= 2;
                        }
                        LOG_DEBUG(_logger) << "JWorker " << _worker_id << " backing off with "
                                           << _assignment->get_name() << ", tries = " << current_tries
                                           << LOG_END;

                        std::this_thread::sleep_for(backoff_duration);
                        retry_duration += backoff_duration;
                        // TODO: Randomize backoff duration?
                    }
                }
            }
        }
        _worker_metrics.update(1, useful_duration, retry_duration, scheduler_duration, idle_duration);
        publish_arrow_metrics(); // Arrow metrics are updated either when the assignment changes,
                                 // or when the ArrowProcessingController asks for them
    }

    _scheduler->last_assignment(_worker_id, _assignment, last_result);

    LOG_DEBUG(_logger) << "JWorker " << _worker_id << " is exiting loop()" << LOG_END;

}

void JWorker::publish_arrow_metrics() {
    if (_assignment != nullptr) {
        auto& source = _arrow_metrics;
        auto& destination = _assignment->get_metrics();
        destination.update(source);
    }
}





