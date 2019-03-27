//
// Created by nbrei on 3/26/19.
//

#include <greenfield/Topology.h>

namespace greenfield {

Topology::ArrowStatus::ArrowStatus(Arrow* arrow) {
    arrow_name = arrow->get_name();
    is_parallel = arrow->is_parallel();
    is_active = arrow->is_active();
    thread_count = arrow->get_thread_count();
    chunksize = arrow->get_chunksize();
    messages_completed = arrow->get_message_count();
    throughput = thread_count / inst_latency;
    avg_latency = arrow->get_total_latency() / messages_completed;
    inst_latency = arrow->get_last_latency();
    avg_overhead = arrow->get_total_overhead() / messages_completed;
    inst_overhead = arrow->get_last_overhead();
}

Topology::~Topology() {

    for (auto component : components) {
        // Topology owns _some_ components, but not necessarily all.
        delete component;
    }
    for (auto pair : arrows) {
        // Topology owns all arrows.
        delete pair.second;
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

void Topology::addArrow(Arrow *arrow) {
    arrows[arrow->get_name()] = arrow;
};

void Topology::activate(std::string arrow_name) {
    auto search = arrows.find(arrow_name);
    if (search == arrows.end()) {
        throw std::runtime_error(arrow_name);
    }
    arrows[arrow_name]->set_active(true);
    arrows[arrow_name]->notify_downstream(true);
}

void Topology::deactivate(std::string arrow_name) {
    auto search = arrows.find(arrow_name);
    if (search == arrows.end()) {
        throw std::runtime_error(arrow_name);
    }
    arrows[arrow_name]->set_active(false);
}


std::vector<Topology::ArrowStatus> Topology::get_arrow_status() {

    std::vector<ArrowStatus> metrics;
    for (auto pair : arrows) {
        metrics.push_back(ArrowStatus(pair.second));
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

    auto search = arrows.find(arrow_name);
    if (search == arrows.end()) {
        throw std::runtime_error(arrow_name);
    }
    return ArrowStatus(search->second);
}

void Topology::log_status() {
    LOG_INFO(logger) << " +--------------------------+-----+-----+---------+-------+-------------+" << LOG_END;
    LOG_INFO(logger) << " |           Name           | Par | Act | Threads | Chunk |  Completed  |" << LOG_END;
    LOG_INFO(logger) << " +--------------------------+-----+-----+---------+-------+-------------+" << LOG_END;
    for (ArrowStatus &as : get_arrow_status()) {
        LOG_INFO(logger) << " | "
                         << std::setw(24) << std::left << as.arrow_name << " | "
                         << std::setw(3) << std::right << (as.is_parallel ? " T " : " F ") << " | "
                         << std::setw(3) << (as.is_active ? " T " : " F ") << " | "
                         << std::setw(7) << as.thread_count << " |"
                         << std::setw(6) << as.chunksize << " |"
                         << std::setw(12) << as.messages_completed << " |"
                         << LOG_END;
    }
    LOG_INFO(logger) << " +--------------------------+-----+-----+---------+-------+-------------+" << LOG_END;
    LOG_INFO(logger)
        << " +--------------------------+------------+-------------+--------------+--------------+---------------+"
        << LOG_END;
    LOG_INFO(logger)
        << " |           Name           | Throughput | Avg latency | Inst latency | Avg overhead | Inst overhead |"
        << LOG_END;
    LOG_INFO(logger)
        << " +--------------------------+------------+-------------+--------------+--------------+---------------+"
        << LOG_END;
    for (ArrowStatus &as : get_arrow_status()) {
        LOG_INFO(logger) << " | "
                         << std::setw(24) << std::left << as.arrow_name << " | "
                         << std::setw(10) << std::right << as.throughput << " |"
                         << std::setw(12) << as.avg_latency << " |"
                         << std::setw(13) << as.inst_latency << " |"
                         << std::setw(13) << as.avg_overhead << " |"
                         << std::setw(14) << as.inst_overhead << " |"
                         << LOG_END;
    }
    LOG_INFO(logger)
        << " +--------------------------+------------+-------------+--------------+--------------+---------------+"
        << LOG_END;
    LOG_INFO(logger) << " +--------------------------+---------+-----------+------+" << LOG_END;
    LOG_INFO(logger) << " |           Name           | Pending | Threshold | More |" << LOG_END;
    LOG_INFO(logger) << " +--------------------------+---------+-----------+------+" << LOG_END;
    for (QueueStatus &qs : get_queue_status()) {
        LOG_INFO(logger) << " | "
                         << std::setw(24) << std::left << qs.queue_name << " |"
                         << std::setw(8) << std::right << qs.message_count << " |"
                         << std::setw(10) << qs.threshold << " |  "
                         << std::setw(1) << (qs.is_active ? "T" : "F") << "   |" << LOG_END;
    }
    LOG_INFO(logger) << " +--------------------------+---------+-----------+------+" << LOG_END;
}


/// The user may want to pause the topology and interact with it manually.
/// This is particularly powerful when used from inside GDB.
StreamStatus Topology::step(const std::string &arrow_name) {
    auto search = arrows.find(arrow_name);
    if (search == arrows.end()) {
        throw std::runtime_error(arrow_name);
    }
    Arrow *arrow = search->second;
    StreamStatus result = arrow->execute();
    return result;
}


} // namespace greenfield