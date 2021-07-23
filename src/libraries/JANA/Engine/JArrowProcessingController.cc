
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Engine/JArrowPerfSummary.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/JLogger.h>

#include <memory>

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

void JArrowProcessingController::acquire_services(JServiceLocator * sl) {
    auto ls = sl->get<JLoggingService>();
    _logger = ls->get_logger("JArrowProcessingController");
    _worker_logger = ls->get_logger("JWorker");
    _scheduler_logger = ls->get_logger("JScheduler");

    // Obtain timeouts from parameter manager
    auto params = sl->get<JParameterManager>();
    params->SetDefaultParameter("jana:timeout", _timeout_s, "Max. time (in seconds) system will wait for a thread to update its heartbeat before killing it and launching a new one.");
	params->SetDefaultParameter("jana:warmup_timeout", _warmup_timeout_s, "Max. time (in seconds) system will wait for the initial events to complete before killing program.");
	// Originally "THREAD_TIMEOUT" and "THREAD_TIMEOUT_FIRST_EVENT"
}

void JArrowProcessingController::initialize() {

    _scheduler = new JScheduler(_topology->arrows);
    _scheduler->logger = _scheduler_logger;
    LOG_INFO(_logger) << _topology->mapping << LOG_END;
}

void JArrowProcessingController::run(size_t nthreads) {

    _topology->set_active(true);
    scale(nthreads);
}

void JArrowProcessingController::scale(size_t nthreads) {

    bool pin_to_cpu = (_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
    size_t next_worker_id = _workers.size();

    while (next_worker_id < nthreads) {

        size_t next_cpu_id = _topology->mapping.get_cpu_id(next_worker_id);
        size_t next_loc_id = _topology->mapping.get_loc_id(next_worker_id);

        auto worker = new JWorker(_scheduler, next_worker_id, next_cpu_id, next_loc_id, pin_to_cpu);
        worker->logger = _worker_logger;
        _workers.push_back(worker);
        next_worker_id++;
    }

    for (size_t i=0; i<nthreads; ++i) {
        _workers.at(i)->start();
    };
    for (size_t i=nthreads; i<next_worker_id; ++i) {
        _workers.at(i)->request_stop();
    }
    for (size_t i=nthreads; i<next_worker_id; ++i) {
        _workers.at(i)->wait_for_stop();
    }
    _topology->metrics.reset();
    _topology->metrics.start(nthreads);
}

void JArrowProcessingController::request_stop() {
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
    // Tell the topology to stop timers and deactivate arrows
    _topology->set_active(false);
}

void JArrowProcessingController::wait_until_stopped() {
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
    for (JWorker* worker : _workers) {
        worker->wait_for_stop();
    }
}

bool JArrowProcessingController::is_stopped() {
    for (JWorker* worker : _workers) {
        if (worker->get_runstate() != JWorker::RunState::Stopped) {
            return false;
        }
    }
    // We have determined that all Workers have actually stopped
    return true;
}

bool JArrowProcessingController::is_finished() {
    return !_topology->is_active();
}

bool JArrowProcessingController::is_timed_out() {

	// Note that this makes its own (redundant) call to measure_internal_performance().
	// Probably want to refactor so that we only make one such call per ticker iteration.
	// Since we are storing our metrics summary anyway, we could call measure_performance()
	// and have print_report(), print_final_report(), is_timed_out(), etc use the cached version
	auto metrics = measure_internal_performance();

	size_t events_in_pool = _topology->event_pool->size();
	bool factories_are_warmed_up = (metrics->total_events_completed >= events_in_pool);

	int timeout_s = (factories_are_warmed_up) ? _timeout_s : _warmup_timeout_s;

	// Find all workers whose last heartbeat exceeds timeout
	bool found_timeout = false;
	for (size_t i=0; i<metrics->workers.size(); ++i) {
		if (metrics->workers[i].last_heartbeat_ms > (timeout_s * 1000)) {
			found_timeout = true;
			_workers[i]->declare_timeout();
			// This assumes the workers and their summaries are ordered the same.
			// Which is true, but I don't like it.
		}
	}
	return found_timeout;
}

JArrowProcessingController::~JArrowProcessingController() {
    request_stop();
    wait_until_stopped();
    for (JWorker* worker : _workers) {
        delete worker;
    }
    delete _topology;
    delete _scheduler;
}

void JArrowProcessingController::print_report() {
    auto metrics = measure_internal_performance();
    LOG_INFO(_logger) << "Running" << *metrics << LOG_END;
}

void JArrowProcessingController::print_final_report() {
    auto metrics = measure_internal_performance();
    LOG_INFO(_logger) << "Final Report" << *metrics << LOG_END;
}

std::unique_ptr<const JArrowPerfSummary> JArrowProcessingController::measure_internal_performance() {

    // Measure perf on all Workers first, as this will prompt them to publish
    // any ArrowMetrics they have collected
    _perf_summary.workers.clear();
    for (JWorker* worker : _workers) {
        WorkerSummary summary;
        worker->measure_perf(summary);
        _perf_summary.workers.push_back(summary);
    }

    size_t monotonic_event_count = 0;
    for (JArrow* arrow : _topology->sinks) {
        monotonic_event_count += arrow->get_metrics().get_total_message_count();
    }

    // Uptime
    _topology->metrics.split(monotonic_event_count);
    _topology->metrics.summarize(_perf_summary);

    double worst_seq_latency = 0;
    double worst_par_latency = 0;

    _perf_summary.arrows.clear();
    for (JArrow* arrow : _topology->arrows) {
        JArrowMetrics::Status last_status;
        size_t total_message_count;
        size_t last_message_count;
        size_t total_queue_visits;
        size_t last_queue_visits;
        JArrowMetrics::duration_t total_latency;
        JArrowMetrics::duration_t last_latency;
        JArrowMetrics::duration_t total_queue_latency;
        JArrowMetrics::duration_t last_queue_latency;

        arrow->get_metrics().get(last_status, total_message_count, last_message_count, total_queue_visits,
                                 last_queue_visits, total_latency, last_latency, total_queue_latency, last_queue_latency);

        auto total_latency_ms = millisecs(total_latency).count();
        auto total_queue_latency_ms = millisecs(total_queue_latency).count();

        ArrowSummary summary;
        summary.arrow_type = arrow->get_type();
        summary.is_parallel = arrow->is_parallel();
        summary.is_active = arrow->is_active();
        summary.thread_count = arrow->get_thread_count();
        summary.arrow_name = arrow->get_name();
        summary.chunksize = arrow->get_chunksize();
        summary.messages_pending = arrow->get_pending();
        summary.is_upstream_active = !arrow->is_upstream_finished();
        summary.threshold = arrow->get_threshold();
        summary.status = arrow->get_status();

        summary.total_messages_completed = total_message_count;
        summary.last_messages_completed = last_message_count;
        summary.queue_visit_count = total_queue_visits;

        summary.avg_queue_latency_ms = (total_queue_visits == 0)
                                       ? std::numeric_limits<double>::infinity()
                                       : total_queue_latency_ms / total_queue_visits;

        summary.avg_queue_overhead_frac = total_queue_latency_ms / (total_queue_latency_ms + total_latency_ms);

        summary.avg_latency_ms = (total_message_count == 0)
                               ? std::numeric_limits<double>::infinity()
                               : summary.avg_latency_ms = total_latency_ms/total_message_count;

        summary.last_latency_ms = (last_message_count == 0)
                                ? std::numeric_limits<double>::infinity()
                                : millisecs(last_latency).count()/last_message_count;

        if (arrow->is_parallel()) {
            worst_par_latency = std::max(worst_par_latency, summary.avg_latency_ms);
        } else {
            worst_seq_latency = std::max(worst_seq_latency, summary.avg_latency_ms);
        }
        _perf_summary.arrows.push_back(summary);
    }

    // bottlenecks
    _perf_summary.avg_seq_bottleneck_hz = 1e3 / worst_seq_latency;
    _perf_summary.avg_par_bottleneck_hz = 1e3 * _perf_summary.thread_count / worst_par_latency;

    auto tighter_bottleneck = std::min(_perf_summary.avg_seq_bottleneck_hz, _perf_summary.avg_par_bottleneck_hz);

    _perf_summary.avg_efficiency_frac = (tighter_bottleneck == 0)
                                      ? std::numeric_limits<double>::infinity()
                                      : _perf_summary.avg_throughput_hz/tighter_bottleneck;

    return std::unique_ptr<JArrowPerfSummary>(new JArrowPerfSummary(_perf_summary));
}

std::unique_ptr<const JPerfSummary> JArrowProcessingController::measure_performance() {
    return measure_internal_performance();
}




