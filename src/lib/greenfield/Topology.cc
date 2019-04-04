//
// Created by nbrei on 3/26/19.
//

#include <greenfield/Topology.h>
#include "ThreadManager.h"

namespace greenfield {

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;


Topology::ArrowStatus::ArrowStatus(Arrow* arrow) {
    arrow_name = arrow->get_name();
    is_parallel = arrow->is_parallel();
    is_active = arrow->is_active();
    thread_count = arrow->get_thread_count();
    chunksize = arrow->get_chunksize();

    clock_t::duration total_latency;
    clock_t::duration queue_overhead;
    clock_t::duration last_latency;
    arrow->get_metrics(messages_completed,
                       queue_visit_count,
                       total_latency,
                       queue_overhead,
                       last_latency);

    double l_total = millisecs(total_latency).count();
    double l_queue = millisecs(queue_overhead).count();
    double l_last = millisecs(last_latency).count();

    avg_latency_ms = l_total / messages_completed;
    inst_latency_ms = l_last;
    queue_overhead_frac = l_queue / (l_queue+l_total);
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
        sinks.push_back(arrow); // So that we can check message_count, is_finished
        attach_upstream(arrow); // So that we can be notified when finish happens
        arrow->attach_downstream(this);
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


Topology::TopologyStatus Topology::get_topology_status(std::map<Arrow*,ArrowStatus>& arrow_statuses) {

    assert(_run_state != RunState::BeforeRun);

    TopologyStatus result;

    // Messages completed
    result.messages_completed = 0;
    for (Arrow* arrow : sinks) {
        result.messages_completed += arrow_statuses.at(arrow).messages_completed;
    }
    uint64_t message_delta = result.messages_completed - _last_message_count;

    // Uptime
    double time_delta;

    if (_run_state == RunState::AfterRun) {
        time_delta = secs(_stop_time - _last_time).count();
        result.uptime_s = secs(_stop_time - _start_time).count();
    }
    else { // RunState::DuringRun
        auto current_time = clock_t::now();
        time_delta = secs(current_time - _last_time).count();
        result.uptime_s = secs(current_time - _start_time).count();
        _last_time = current_time;
        _last_message_count = result.messages_completed;
    }

    // Throughput
    result.avg_throughput_hz = result.messages_completed / result.uptime_s;
    result.inst_throughput_hz = message_delta / time_delta;

    // bottlenecks
    double worst_seq_latency = 0;
    double worst_par_latency = 0;
    for (Arrow* arrow : arrows) {
        if (arrow->is_parallel()) {
            worst_par_latency = std::max(worst_par_latency, arrow_statuses.at(arrow).avg_latency_ms);
        } else {
            worst_seq_latency = std::max(worst_seq_latency, arrow_statuses.at(arrow).avg_latency_ms);
        }
    }
    result.seq_bottleneck_hz = 1e3 / worst_seq_latency;
    result.par_bottleneck_hz = 1e3 * _ncpus / worst_par_latency;

    auto tighter_bottleneck = std::min(result.seq_bottleneck_hz, result.par_bottleneck_hz);
    result.efficiency_frac = result.avg_throughput_hz/tighter_bottleneck;

    return result;

}

Topology::TopologyStatus Topology::get_topology_status() {
    std::map<Arrow*, ArrowStatus> statuses;
    for (Arrow* arrow : arrows) {
        statuses.insert({arrow, ArrowStatus(arrow)});
    }
    return get_topology_status(statuses);
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

    std::map<Arrow*, ArrowStatus> statuses;
    for (Arrow* arrow : arrows) {
        statuses.insert({arrow, ArrowStatus(arrow)});
    }

    std::ostringstream os;
    if (_run_state != RunState::BeforeRun) {

        auto s = get_topology_status(statuses);

        os << std::endl;
        os << " TOPOLOGY STATUS" << std::endl;
        os << " ---------------" << std::endl;
        os << " Thread team size [count]:    " << _ncpus << std::endl;
        os << " Uptime [s]:                  " << std::setprecision(4) << s.uptime_s << std::endl;
        os << " Completed events [count]:    " << s.messages_completed << std::endl;
        os << " Avg throughput [Hz]:         " << std::setprecision(3) << s.avg_throughput_hz << std::endl;
        os << " Inst throughput [Hz]:        " << std::setprecision(3) << s.inst_throughput_hz << std::endl;
        os << " Sequential bottleneck [Hz]:  " << std::setprecision(3) << s.seq_bottleneck_hz << std::endl;
        os << " Parallel bottleneck [Hz]:    " << std::setprecision(3) << s.par_bottleneck_hz << std::endl;
        os << " Efficiency [0..1]:           " << std::setprecision(3) << s.efficiency_frac << std::endl;
        os << std::endl;
    }

    os << " ARROW STATUS" << std::endl;
    os << " +--------------------------+-----+-----+---------+-------+-------------+" << std::endl;
    os << " |           Name           | Par | Act | Threads | Chunk |  Completed  |" << std::endl;
    os << " +--------------------------+-----+-----+---------+-------+-------------+" << std::endl;
    for (Arrow* arrow : arrows) {
        ArrowStatus& as = statuses.at(arrow);
        os << " | "
                         << std::setw(24) << std::left << as.arrow_name << " | "
                         << std::setw(3) << std::right << (as.is_parallel ? " T " : " F ") << " | "
                         << std::setw(3) << (as.is_active ? " T " : " F ") << " | "
                         << std::setw(7) << as.thread_count << " |"
                         << std::setw(6) << as.chunksize << " |"
                         << std::setw(12) << as.messages_completed << " |"
                         << std::endl;
    }
    os << " +--------------------------+-----+-----+---------+-------+-------------+" << std::endl;

    if (_run_state != RunState::BeforeRun) {

        os << " +--------------------------+-------------+--------------+----------------+--------------+" << std::endl;
        os << " |           Name           | Avg latency | Inst latency | Queue overhead | Queue visits | " << std::endl;
        os << " |                          | [ms/event]  |  [ms/event]  |     [0..1]     |    [count]   | " << std::endl;
        os << " +--------------------------+-------------+--------------+----------------+--------------+" << std::endl;

        for (Arrow* arrow : arrows) {
            ArrowStatus& as = statuses.at(arrow);
            os << " | " << std::setprecision(3)
                             << std::setw(24) << std::left << as.arrow_name << " | "
                             << std::setw(11) << std::right << as.avg_latency_ms << " |"
                             << std::setw(13) << as.inst_latency_ms << " |"
                             << std::setw(15) << as.queue_overhead_frac << " |"
                             << std::setw(13) << as.queue_visit_count << " |"
                             << std::endl;
        }
        os << " +--------------------------+-------------+--------------+----------------+--------------+" << std::endl;

    }
    os << std::endl;
    os << " QUEUE STATUS" << std::endl;
    os << " +--------------------------+---------+-----------+------+" << std::endl;
    os << " |           Name           | Pending | Threshold | More |" << std::endl;
    os << " +--------------------------+---------+-----------+------+" << std::endl;
    for (QueueStatus &qs : get_queue_status()) {
        os << " | " << std::setw(24) << std::left << qs.queue_name << " |"
           << std::setw(8) << std::right << qs.message_count << " |"
           << std::setw(10) << qs.threshold << " |  "
           << std::setw(1) << (qs.is_active ? "T" : "F") << "   |" << std::endl;
    }
    os << " +--------------------------+---------+-----------+------+" << std::endl;
    os << std::endl;

    if (_run_state == RunState::DuringRun) {
        os << " WORKER STATUS" << std::endl;
        os << " +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;
        os << " | ID | Last arrow name      | Useful time | Retry time | Idle time | Scheduler time | Scheduler visits |" << std::endl;
        os << " |    |                      |    [0..1]   |   [0..1]   |   [0..1]  |     [0..1]     |     [count]      |" << std::endl;
        os << " +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;
        for (Worker::Summary ws : _threadManager->get_worker_summaries()) {
            os << " |"
               << std::setw(3) << std::right << ws.worker_id << " | "
               << std::setw(20) << std::left << ws.last_arrow_name << " |"
               << std::setw(12) << std::right << ws.useful_time_frac << " |"
               << std::setw(11) << ws.retry_time_frac << " |"
               << std::setw(10) << ws.idle_time_frac << " |"
               << std::setw(15) << ws.scheduler_time_frac << " |"
               << std::setw(17) << ws.scheduler_visits << " |"
               << std::endl;
        }
        os << " +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;
    }
    LOG_INFO(logger) << os.str() << LOG_END;
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

void Topology::set_active(bool active) {

    if (active) {
        // activate any sources, eventually
    }
    else {
        if (_run_state == RunState::DuringRun) {
            _stop_time = clock_t::now();
            _run_state = RunState::AfterRun;
        }
    }
}

class RoundRobinScheduler;

void Topology::run(int nthreads) {

    _scheduler = new RoundRobinScheduler(*this);
    //_scheduler->logger = Logger::everything();
    _threadManager = new ThreadManager(*_scheduler);
    //_threadManager->logger = Logger::everything();
    _threadManager->run(nthreads);
    _run_state = RunState::DuringRun;
    _start_time = std::chrono::steady_clock::now();
    _last_time = _start_time;
    _ncpus = nthreads;
}

void Topology::wait_until_finished() {
    if (_threadManager != nullptr) {
        while (is_active()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        if (_run_state == RunState::DuringRun) {
            // We shouldn't usually end up here because sinks notify us when
            // they deactivate, automatically calling Topology::set_active(false),
            // which stops the clock as soon as possible.
            // However, if for unknown reasons nobody notifies us, we still want to change
            // run state in an orderly fashion. If we do end up here, though, our _stop_time
            // will be late, throwing our metrics off.
            _run_state = RunState::AfterRun;
            _stop_time = clock_t::now();
        }
        _threadManager->stop();
        _threadManager->join();
    }
}

} // namespace greenfield