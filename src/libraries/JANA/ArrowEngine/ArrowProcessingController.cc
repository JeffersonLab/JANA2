
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/ArrowEngine/ArrowProcessingController.h>
#include <JANA/Engine/JArrowPerfSummary.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/JLogger.h>

#include <memory>

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

void ArrowProcessingController::acquire_services(JServiceLocator * sl) {
    auto ls = sl->get<JLoggingService>();
    _logger = ls->get_logger("JArrowProcessingController");
    _worker_logger = ls->get_logger("JWorker");
    _scheduler_logger = ls->get_logger("JScheduler");
}

void ArrowProcessingController::initialize() {

    _scheduler = new JScheduler(_topology->arrows);
    _scheduler->logger = _scheduler_logger;
    LOG_INFO(_logger) << _topology->mapping << LOG_END;
}

void ArrowProcessingController::run(size_t nthreads) {

    _topology->set_active(true);
    scale(nthreads);
}

void ArrowProcessingController::scale(size_t nthreads) {

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

void ArrowProcessingController::request_stop() {
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
    // Tell the topology to stop timers and deactivate arrows
    _topology->set_active(false);
}

void ArrowProcessingController::wait_until_stopped() {
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
    for (JWorker* worker : _workers) {
        worker->wait_for_stop();
    }
}

bool ArrowProcessingController::is_stopped() {
    for (JWorker* worker : _workers) {
        if (worker->get_runstate() != JWorker::RunState::Stopped) {
            return false;
        }
    }
    // We have determined that all Workers have actually stopped
    return true;
}

bool ArrowProcessingController::is_finished() {
    return !_topology->is_active();
}

ArrowProcessingController::~ArrowProcessingController() {
    request_stop();
    wait_until_stopped();
    for (JWorker* worker : _workers) {
        delete worker;
    }
    delete _topology;
    delete _scheduler;
}

void ArrowProcessingController::print_report() {
    auto metrics = measure_internal_performance();
    jout << "Running" << *metrics << jendl;
}

void ArrowProcessingController::print_final_report() {
    auto metrics = measure_internal_performance();
    jout << "Final Report" << *metrics << jendl;
}

std::unique_ptr<const JArrowPerfSummary> ArrowProcessingController::measure_internal_performance() {

    // Measure perf on all Workers first, as this will prompt them to publish
    // any ArrowMetrics they have collected
    _perf_summary.workers.clear();
    for (JWorker* worker : _workers) {
        WorkerSummary summary;
        worker->measure_perf(summary);
        _perf_summary.workers.push_back(summary);
    }

    size_t monotonic_event_count = 0;
    for (Arrow* arrow : _topology->sinks) {
        monotonic_event_count += arrow->get_metrics().get_total_message_count();
    }

    // Uptime
    _topology->metrics.split(monotonic_event_count);
    _topology->metrics.summarize(_perf_summary);

    double worst_seq_latency = 0;
    double worst_par_latency = 0;

    _perf_summary.arrows.clear();
    for (Arrow* arrow : _topology->arrows) {
        ArrowMetrics::Status last_status;
        size_t total_message_count;
        size_t last_message_count;
        size_t total_queue_visits;
        size_t last_queue_visits;
        duration_t total_latency;
        duration_t last_latency;
        duration_t total_queue_latency;
        duration_t last_queue_latency;

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

    return std::unique_ptr<ArrowPerfSummary>(new ArrowPerfSummary(_perf_summary));
}

std::unique_ptr<const JPerfSummary> ArrowProcessingController::measure_performance() {
    return measure_internal_performance();
}



