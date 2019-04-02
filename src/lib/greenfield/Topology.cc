//
// Created by nbrei on 3/26/19.
//

#include <greenfield/Topology.h>
#include "ThreadManager.h"

namespace greenfield {

Topology::ArrowStatus::ArrowStatus(Arrow* arrow) {
    arrow_name = arrow->get_name();
    is_parallel = arrow->is_parallel();
    is_active = arrow->is_active();
    thread_count = arrow->get_thread_count();
    chunksize = arrow->get_chunksize();
    messages_completed = arrow->get_message_count();

    auto latency = arrow->get_total_latency();
    avg_latency_ms = latency / messages_completed / 1.0e6;
    inst_latency_ms = arrow->get_last_latency() / 1.0e6;

    auto overhead = arrow->get_total_overhead();
    queue_overhead_frac = overhead/(overhead+latency);
    queue_visit_count = arrow->get_queue_visits();
}

Topology::~Topology() {

    arrow_lookup.clear();

    for (auto component : components) {
        // Topology owns _some_ components, but not necessarily all.
        delete component;
    }
    for (auto arrow : arrows) {
        // Topology owns all arrows.
        delete arrow;
    }
    for (auto queue : queues) {
        // Topology owns all queues.
        delete queue;
    }
}

void Topology::addManagedComponent(Component *component) {
    components.push_back(component);
}

void Topology::addQueue(QueueBase *queue) {
    queues.push_back(queue);
}

void Topology::addArrow(Arrow *arrow, bool sink) {
    arrows.push_back(arrow);
    arrow_lookup[arrow->get_name()] = arrow;
    if (sink) {
        sinks.push_back(arrow);
    }
};

Arrow* Topology::get_arrow(std::string arrow_name) {

    auto search = arrow_lookup.find(arrow_name);
    if (search == arrow_lookup.end()) {
        throw std::runtime_error(arrow_name);
    }
    return search->second;
}

void Topology::activate(std::string arrow_name) {
    auto arrow = get_arrow(arrow_name);
    arrow->set_active(true);
    arrow->notify_downstream(true);
}

void Topology::deactivate(std::string arrow_name) {
    auto arrow = get_arrow(arrow_name);
    arrow->set_active(false);
}


Topology::TopologyStatus Topology::get_topology_status() {
    TopologyStatus result;

    // Messages completed
    result.messages_completed = 0;
    for (Arrow* arrow : sinks) {
        result.messages_completed += arrow->get_message_count();
    }
    uint64_t message_delta = result.messages_completed - _last_message_count;
    _last_message_count = result.messages_completed;

    // Uptime
    auto current_time = std::chrono::steady_clock::now();
    double time_delta = std::chrono::duration<float>(current_time - _last_time).count();
    _last_time = current_time;
    result.uptime_s = (current_time - _start_time).count() / 1e9;

    // Throughput
    result.avg_throughput_hz = result.messages_completed / result.uptime_s;
    result.inst_throughput_hz = (message_delta * 1.0) / time_delta;

    // Sequential bottleneck
    double worst_seq_latency = 0;
    for (Arrow* arrow : arrows) {
        if (!arrow->is_parallel()) {
            worst_seq_latency = std::max(worst_seq_latency, arrow->get_total_latency()/arrow->get_message_count());
        }
    }
    result.seq_bottleneck_hz = 1e9 / worst_seq_latency;
    result.efficiency_frac = result.avg_throughput_hz/result.seq_bottleneck_hz;

    // Scheduler visits
    result.scheduler_visit_count = _scheduler_visits;
    if (result.uptime_s == 0) {
        result.scheduler_overhead_frac = 0;
    }
    else {
        auto scheduler_time = std::chrono::duration<double>(_scheduler_time);
        result.scheduler_overhead_frac = scheduler_time.count() / (result.uptime_s * _ncpus);
    }

    return result;

}


std::vector<Topology::ArrowStatus> Topology::get_arrow_status() {

    std::vector<ArrowStatus> metrics;
    for (auto arrow : arrows) {
        metrics.push_back(ArrowStatus(arrow));
    }
    return metrics;
}

std::vector<Topology::QueueStatus> Topology::get_queue_status() {
    std::vector<QueueStatus> statuses;
    for (QueueBase *q : queues) {
        QueueStatus qs;
        qs.queue_name = q->get_name();
        qs.is_active = q->is_active();
        qs.message_count = q->get_item_count();
        qs.threshold = q->get_threshold();
        statuses.push_back(qs);
    }
    return statuses;
}

Topology::ArrowStatus Topology::get_status(const std::string &arrow_name) {

    return ArrowStatus(get_arrow(arrow_name));
}

void Topology::log_status() {

    auto s = get_topology_status();

    LOG_INFO(logger) << "TOPOLOGY STATUS" << LOG_END;
    LOG_INFO(logger) << "---------------" << LOG_END;
    LOG_INFO(logger) << "Uptime [s]:                  " << std::setprecision(4) << s.uptime_s << LOG_END;
    LOG_INFO(logger) << "Completed events [count]:    " << s.messages_completed << LOG_END;
    LOG_INFO(logger) << "Avg throughput [Hz]:         " << std::setprecision(3) << s.avg_throughput_hz << LOG_END;
    LOG_INFO(logger) << "Inst throughput [Hz]:        " << std::setprecision(3) << s.inst_throughput_hz << LOG_END;
    LOG_INFO(logger) << "Sequential bottleneck [Hz]:  " << std::setprecision(3) << s.seq_bottleneck_hz << LOG_END;
    LOG_INFO(logger) << "Efficiency [0..1]:           " << std::setprecision(3) << s.efficiency_frac << LOG_END;
    LOG_INFO(logger) << LOG_END;
    LOG_INFO(logger) << "Scheduler visits [count]:          " << s.scheduler_visit_count << LOG_END;
    LOG_INFO(logger) << "Scheduler overhead [0..1]:         " << std::setprecision(3) << s.scheduler_overhead_frac << LOG_END;
    LOG_INFO(logger) << LOG_END;
    LOG_INFO(logger) << "ARROW STATUS" << LOG_END;
    auto statuses = get_arrow_status();
    LOG_INFO(logger) << "+--------------------------+-----+-----+---------+-------+-------------+" << LOG_END;
    LOG_INFO(logger) << "|           Name           | Par | Act | Threads | Chunk |  Completed  |" << LOG_END;
    LOG_INFO(logger) << "+--------------------------+-----+-----+---------+-------+-------------+" << LOG_END;
    for (ArrowStatus &as : statuses) {
        LOG_INFO(logger) << "| "
                         << std::setw(24) << std::left << as.arrow_name << " | "
                         << std::setw(3) << std::right << (as.is_parallel ? " T " : " F ") << " | "
                         << std::setw(3) << (as.is_active ? " T " : " F ") << " | "
                         << std::setw(7) << as.thread_count << " |"
                         << std::setw(6) << as.chunksize << " |"
                         << std::setw(12) << as.messages_completed << " |"
                         << LOG_END;
    }
    LOG_INFO(logger) << "+--------------------------+-----+-----+---------+-------+-------------+" << LOG_END;
    LOG_INFO(logger)
        << "+--------------------------+-------------+--------------+----------------+--------------+" << LOG_END;
    LOG_INFO(logger)
        << "|           Name           | Avg latency | Inst latency | Queue overhead | Queue visits | " << LOG_END;
    LOG_INFO(logger)
        << "|                          | [ms/event]  |  [ms/event]  |     [0..1]     |    [count]   | "
        << LOG_END;
    LOG_INFO(logger)
        << "+--------------------------+-------------+--------------+----------------+--------------+" << LOG_END;

    for (ArrowStatus &as : statuses) {
        LOG_INFO(logger) << "| " << std::setprecision(3)
                         << std::setw(24) << std::left << as.arrow_name << " | "
                         << std::setw(11) << std::right << as.avg_latency_ms << " |"
                         << std::setw(13) << as.inst_latency_ms << " |"
                         << std::setw(15) << as.queue_overhead_frac << " |"
                         << std::setw(13) << as.queue_visit_count << " |"
                         << LOG_END;
    }
    LOG_INFO(logger)
        << "+--------------------------+-------------+--------------+----------------+--------------+" << LOG_END;

    LOG_INFO(logger) << "QUEUE STATUS" << LOG_END;
    LOG_INFO(logger) << "+--------------------------+---------+-----------+------+" << LOG_END;
    LOG_INFO(logger) << "|           Name           | Pending | Threshold | More |" << LOG_END;
    LOG_INFO(logger) << "+--------------------------+---------+-----------+------+" << LOG_END;
    for (QueueStatus &qs : get_queue_status()) {
        LOG_INFO(logger) << "| "
                         << std::setw(24) << std::left << qs.queue_name << " |"
                         << std::setw(8) << std::right << qs.message_count << " |"
                         << std::setw(10) << qs.threshold << " |  "
                         << std::setw(1) << (qs.is_active ? "T" : "F") << "   |" << LOG_END;
    }
    LOG_INFO(logger) << "+--------------------------+---------+-----------+------+" << LOG_END;

    LOG_INFO(logger) << LOG_END;

    if (_threadManager != nullptr) {
        LOG_INFO(logger) << "WORKER STATUS" << LOG_END;
        LOG_INFO(logger) << "+----+---------+---------------------+" << LOG_END;
        LOG_INFO(logger) << "| ID | Running | Arrow name          |" << LOG_END;
        LOG_INFO(logger) << "+----+---------+---------------------+" << LOG_END;
        for (ThreadManager::WorkerStatus ws : _threadManager->get_worker_statuses()) {
            LOG_INFO(logger) << "|"
                             << std::setw(3) << ws.worker_id << " |"
                             << std::setw(7) << ((ws.is_running) ? "    T   " : "    F   ") << " | "
                             << std::setw(19) << std::left << ws.arrow_name << " |" << LOG_END;
        }
        LOG_INFO(logger) << "+----+---------+---------------------+" << LOG_END;
    }
}


/// The user may want to pause the topology and interact with it manually.
/// This is particularly powerful when used from inside GDB.
StreamStatus Topology::step(const std::string &arrow_name) {
    Arrow *arrow = get_arrow(arrow_name);
    StreamStatus result = arrow->execute();
    return result;
}

bool Topology::is_active() {
    for (auto arrow : arrows) {
        if (arrow->is_active()) {
            return true;
        }
    }
    return false;
}

class RoundRobinScheduler;

void Topology::run(int nthreads) {

    _scheduler = new RoundRobinScheduler(*this);
    //_scheduler->logger = Logger::everything();
    _threadManager = new ThreadManager(*_scheduler);
    //_threadManager->logger = Logger::everything();
    _threadManager->run(nthreads);
    _start_time = std::chrono::steady_clock::now();
    _last_time = _start_time;
    _ncpus = nthreads;
}



} // namespace greenfield