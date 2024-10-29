
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Engine/JPerfSummary.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/JLogger.h>

#include <memory>

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

void JArrowProcessingController::acquire_services(JServiceLocator * sl) {

    auto params = sl->get<JParameterManager>();
    m_topology = sl->get<JTopologyBuilder>();

    params->SetDefaultParameter("jana:timeout", m_timeout_s, "Max time (in seconds) JANA will wait for a thread to update its heartbeat before hard-exiting. 0 to disable timeout completely.");
    params->SetDefaultParameter("jana:warmup_timeout", m_warmup_timeout_s, "Max time (in seconds) JANA will wait for 'initial' events to complete before hard-exiting.");
    // Originally "THREAD_TIMEOUT" and "THREAD_TIMEOUT_FIRST_EVENT"
}

void JArrowProcessingController::initialize() {

    m_scheduler = new JScheduler(m_topology);
    m_scheduler->logger = GetLogger();
    LOG_INFO(GetLogger()) << m_topology->mapping << LOG_END;

    m_scheduler->initialize_topology();

}

/// @brief This will run the JArrowTopology and create N JWorker objects, each with thier own threads.
///
/// This will first call JTopology::run(size_t) to initialize the topology and
/// prepare it for workers. It will then create `nthreads` JWorker objects and call
/// their JWorker::start() methods causing them to create their own threads and
/// start running their JWorker::loop() methods. This will return after the worker
/// threads are launched.
///
/// @param [in] nthreads The number of worker threads to start
void JArrowProcessingController::run(size_t nthreads) {
    LOG_INFO(GetLogger()) << "run(): Launching " << nthreads << " workers" << LOG_END;
    // run_topology needs to happen _before_ threads are started so that threads don't quit due to lack of assignments
    m_scheduler->run_topology(nthreads);

    bool pin_to_cpu = (m_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);

    size_t next_worker_id = 0;
    while (next_worker_id < nthreads) {
        size_t next_cpu_id = m_topology->mapping.get_cpu_id(next_worker_id);
        size_t next_loc_id = m_topology->mapping.get_loc_id(next_worker_id);
        auto worker = new JWorker(this, m_scheduler, next_worker_id, next_cpu_id, next_loc_id, pin_to_cpu);
        worker->logger = GetLogger();
        m_workers.push_back(worker);
        next_worker_id++;
    }
    for (size_t i=0; i<nthreads; ++i) {
        m_workers.at(i)->start();
    };
    // It's tempting to put a barrier here so that JAPC::run() blocks until all workers have entered loop().
    // The reason it doesn't work is that the topology might exit immediately (or close to immediately), leaving
    // the supervisor thread waiting forever for workers to reach RunState::Running when they've already Stopped.
}

void JArrowProcessingController::scale(size_t nthreads) {

    LOG_INFO(GetLogger()) << "scale(): Stopping all running workers" << LOG_END;
    m_scheduler->request_topology_pause();
    for (JWorker* worker : m_workers) {
        worker->wait_for_stop();
    }
    m_scheduler->achieve_topology_pause();

    LOG_INFO(GetLogger()) << "scale(): All workers are stopped" << LOG_END;
    bool pin_to_cpu = (m_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
    size_t next_worker_id = m_workers.size();

    while (next_worker_id < nthreads) {

        size_t next_cpu_id = m_topology->mapping.get_cpu_id(next_worker_id);
        size_t next_loc_id = m_topology->mapping.get_loc_id(next_worker_id);

        auto worker = new JWorker(this, m_scheduler, next_worker_id, next_cpu_id, next_loc_id, pin_to_cpu);
        worker->logger = GetLogger();
        m_workers.push_back(worker);
        next_worker_id++;
    }

    LOG_INFO(GetLogger()) << "scale(): Restarting " << nthreads << " workers" << LOG_END;
    // topology->run needs to happen _before_ threads are started so that threads don't quit due to lack of assignments
    m_scheduler->run_topology(nthreads);

    for (size_t i=0; i<nthreads; ++i) {
        m_workers.at(i)->start();
    };
}

void JArrowProcessingController::request_pause() {
    m_scheduler->request_topology_pause();
    // Or:
    // for (JWorker* worker : m_workers) {
    //     worker->request_stop();
    // }
}

void JArrowProcessingController::wait_until_paused() {
    for (JWorker* worker : m_workers) {
        worker->wait_for_stop();
    }
    // Join all the worker threads.
    // Do not trigger the pause (we may want the pause to come internally, e.g. from an event source running out.)
    // Do NOT finish() the topology (we want the ability to be able to restart it)
    m_scheduler->achieve_topology_pause();
}

void JArrowProcessingController::request_stop() {
    // Shut off the sources; the workers will stop on their own once they run out of assignments.
    // Unlike request_pause, this drains all queues.
    // Conceivably, a user could request_stop() followed by wait_until_paused(), which would drain all queues but
    // leave the topology in a restartable state. This suggests we might want to rename some of these things.
    // e.g. request_stop      =>   drain OR drain_then_pause
    //      request_pause     =>   pause
    //      wait_until_stop   =>   join_then_finish
    //      wait_until_pause  =>   join
    m_scheduler->drain_topology();
}

void JArrowProcessingController::wait_until_stopped() {
    // Join all workers
    for (JWorker* worker : m_workers) {
        worker->wait_for_stop();
    }
    // finish out the topology
    // (note some arrows might have already finished e.g. event sources, but that's fine, finish() is idempotent)
    m_scheduler->achieve_topology_pause();
    m_scheduler->finish_topology();
}

bool JArrowProcessingController::is_stopped() {
    return m_scheduler->get_topology_status() == JScheduler::TopologyStatus::Paused;
}

bool JArrowProcessingController::is_finished() {
    return m_scheduler->get_topology_status() == JScheduler::TopologyStatus::Finalized;
}

bool JArrowProcessingController::is_timed_out() {
    if (m_timeout_s == 0) return false;

    // Note that this makes its own (redundant) call to measure_internal_performance().
    // Probably want to refactor so that we only make one such call per ticker iteration.
    // Since we are storing our metrics summary anyway, we could call measure_performance()
    // and have print_report(), print_final_report(), is_timed_out(), etc use the cached version
    auto metrics = measure_performance();

    int timeout_s;
    if (metrics->total_uptime_s < (m_warmup_timeout_s * m_topology->m_max_inflight_events * 1.0) / metrics->thread_count) {
        // We are at the beginning and not all events have necessarily had a chance to warm up
        timeout_s = m_warmup_timeout_s;
    }
    else {
        timeout_s = m_timeout_s;
    }

    // Find all workers whose last heartbeat exceeds timeout
    bool found_timeout = false;
    for (size_t i=0; i<metrics->workers.size(); ++i) {
        if (metrics->workers[i].last_heartbeat_ms > (timeout_s * 1000)) {
            found_timeout = true;
            m_workers[i]->declare_timeout();
            // This assumes the workers and their summaries are ordered the same.
            // Which is true, but I don't like it.
        }
    }
    return found_timeout;
}

bool JArrowProcessingController::is_excepted() {
    for (auto worker : m_workers) {
        if (worker->get_runstate() == JWorker::RunState::Excepted) {
            return true;
        }
    }
    return false;
}

std::vector<JException> JArrowProcessingController::get_exceptions() const {
    std::vector<JException> exceptions;
    for (auto worker : m_workers) {
        if (worker->get_runstate() == JWorker::RunState::Excepted) {
            exceptions.push_back(worker->get_exception());
        }
    }
    return exceptions;
}

JArrowProcessingController::~JArrowProcessingController() {

    for (JWorker* worker : m_workers) {
        worker->request_stop();
    }
    for (JWorker* worker : m_workers) {
        worker->wait_for_stop();
    }
    for (JWorker* worker : m_workers) {
        delete worker;
    }
    delete m_scheduler;
}

JArrowMetrics::Status JArrowProcessingController::execute_arrow(int arrow_index) {
    auto arrow = m_scheduler->checkout(arrow_index);
    if (arrow == nullptr) return JArrowMetrics::Status::Error;
    JArrowMetrics metrics;
    arrow->execute(metrics, 0);
    m_scheduler->last_assignment(0, arrow, metrics.get_last_status());
    return metrics.get_last_status();
}

void JArrowProcessingController::print_report() {
    auto metrics = measure_performance();
    LOG_INFO(GetLogger()) << "Running" << *metrics << LOG_END;
}

void JArrowProcessingController::print_final_report() {
    auto metrics = measure_performance();

    LOG_INFO(GetLogger()) << "Detailed report:" << LOG_END;
    LOG_INFO(GetLogger()) << LOG_END;
    LOG_INFO(GetLogger()) << "  Thread team size [count]:    " << metrics->thread_count << LOG_END;
    LOG_INFO(GetLogger()) << "  Total uptime [s]:            " << std::setprecision(4) << metrics->total_uptime_s << LOG_END;
    LOG_INFO(GetLogger()) << "  Completed events [count]:    " << metrics->total_events_completed << LOG_END;
    LOG_INFO(GetLogger()) << "  Avg throughput [Hz]:         " << std::setprecision(3) << metrics->avg_throughput_hz << LOG_END;
    LOG_INFO(GetLogger()) << "  Sequential bottleneck [Hz]:  " << std::setprecision(3) << metrics->avg_seq_bottleneck_hz << LOG_END;
    LOG_INFO(GetLogger()) << "  Parallel bottleneck [Hz]:    " << std::setprecision(3) << metrics->avg_par_bottleneck_hz << LOG_END;
    LOG_INFO(GetLogger()) << "  Efficiency [0..1]:           " << std::setprecision(3) << metrics->avg_efficiency_frac << LOG_END;

    if (!metrics->arrows.empty()) {
        LOG_INFO(GetLogger()) << LOG_END;
        LOG_INFO(GetLogger()) << "  Arrow-level metrics:" << LOG_END;
        LOG_INFO(GetLogger()) << LOG_END;
    }
    for (auto& as : metrics->arrows) {
        LOG_INFO(GetLogger()) << "  - Arrow name:                        " << as.arrow_name << LOG_END;
        LOG_INFO(GetLogger()) << "    Events completed:                  " << as.total_messages_completed << LOG_END;
        LOG_INFO(GetLogger()) << "    Avg processing latency [ms/event]: " << as.avg_latency_ms << LOG_END;
        LOG_INFO(GetLogger()) << "    Avg queue latency [ms/event]:      " << as.avg_queue_latency_ms << LOG_END;
        LOG_INFO(GetLogger()) << "    Total queue visits [count]:        " << as.queue_visit_count << LOG_END;
        LOG_INFO(GetLogger()) << "    Queue overhead [0..1]:             " << as.avg_queue_overhead_frac << LOG_END;
        LOG_INFO(GetLogger()) << LOG_END;
    }

    LOG_WARN(GetLogger()) << "Final report: " << metrics->total_events_completed << " events processed at "
                       << JTypeInfo::to_string_with_si_prefix(metrics->avg_throughput_hz) << "Hz" << LOG_END;
}

std::unique_ptr<const JPerfSummary> JArrowProcessingController::measure_performance() {

    // Measure perf on all Workers first, as this will prompt them to publish
    // any ArrowMetrics they have collected
    if (m_perf_summary.workers.size() != m_workers.size()) {
        m_perf_summary.workers = std::vector<WorkerSummary>(m_workers.size());
    }
    for (size_t i=0; i<m_workers.size(); ++i) {
        m_workers[i]->measure_perf(m_perf_summary.workers[i]);
    }

    size_t monotonic_event_count = 0;
    for (JArrow* arrow : m_topology->arrows) {
        if (arrow->is_sink()) {
            monotonic_event_count += arrow->get_metrics().get_total_message_count();
        }
    }

    // Uptime
    m_topology->metrics.split(monotonic_event_count);
    m_topology->metrics.summarize(m_perf_summary);

    double worst_seq_latency = 0;
    double worst_par_latency = 0;

    m_scheduler->summarize_arrows(m_perf_summary.arrows);
    

    // Figure out what the bottlenecks in this topology are
    for (const ArrowSummary& summary : m_perf_summary.arrows) {
        if (summary.is_parallel) {
            worst_par_latency = std::max(worst_par_latency, summary.avg_latency_ms);
        } else {
            worst_seq_latency = std::max(worst_seq_latency, summary.avg_latency_ms);
        }
    }

    // bottlenecks
    m_perf_summary.avg_seq_bottleneck_hz = 1e3 / worst_seq_latency;
    m_perf_summary.avg_par_bottleneck_hz = 1e3 * m_perf_summary.thread_count / worst_par_latency;

    auto tighter_bottleneck = std::min(m_perf_summary.avg_seq_bottleneck_hz, m_perf_summary.avg_par_bottleneck_hz);

    m_perf_summary.avg_efficiency_frac = (tighter_bottleneck == 0)
                                      ? std::numeric_limits<double>::infinity()
                                      : m_perf_summary.avg_throughput_hz / tighter_bottleneck;

    return std::unique_ptr<JPerfSummary>(new JPerfSummary(m_perf_summary));
}





