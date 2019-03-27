//
// Created by nbrei on 3/26/19.
//

#include <greenfield/Topology.h>

namespace greenfield {

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
    queue->set_id(queues.size());
    queues.push_back(queue);
}

void Topology::addArrow(Arrow *arrow) {
    arrows[arrow->get_name()] = arrow;
};

void Topology::finalize() {
}

void Topology::activate(std::string arrow_name) {
    arrows[arrow_name]->set_active(true);
}

void Topology::deactivate(std::string arrow_name) {
    arrows[arrow_name]->set_active(false);
}

std::vector<Topology::ArrowStatus> Topology::get_arrow_status() {

    std::vector<ArrowStatus> metrics;

    for (auto pair : arrows) {
        Arrow* arrow = pair.second;
        ArrowStatus status;
        status.arrow_name = arrow->get_name();
        status.is_finished = !arrow->is_active();
        status.thread_count = arrow->get_thread_count();
        status.messages_completed = arrow->get_message_count();
        status.is_parallel = arrow->is_parallel();
        status.long_term_avg_latency = arrow->get_total_latency() / status.messages_completed;
        status.short_term_avg_latency = arrow->get_last_latency();
        metrics.push_back(status);
    }
    return metrics;
}

std::vector<Topology::QueueStatus> Topology::get_queue_status() {
    std::vector<QueueStatus> statuses;
    int i = 0;
    for (QueueBase *q : queues) {
        QueueStatus qs;
        qs.queue_id = i++;
        qs.is_finished = !q->is_active();
        qs.message_count = q->get_item_count();
        qs.message_count_threshold = q->get_threshold();
        statuses.push_back(qs);
    }
    return statuses;
}

void Topology::log_arrow_status() {
    LOG_INFO(logger)
        << "  +--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+"
        << LOG_END;
    LOG_INFO(logger)
        << "  |              Name              | Parallel | Threads | Messages completed | Latency (short) | Latency (long) | Finished |"
        << LOG_END;
    LOG_INFO(logger)
        << "  +--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+"
        << LOG_END;
    for (ArrowStatus &as : get_arrow_status()) {
        LOG_INFO(logger) << "  | "
                         << std::setw(30) << std::left << as.arrow_name << " | "
                         << std::setw(8) << std::right << as.is_parallel << " | "
                         << std::setw(7) << as.thread_count << " |"
                         << std::setw(19) << as.messages_completed << " |"
                         << std::setw(16) << as.short_term_avg_latency << " |"
                         << std::setw(15) << as.long_term_avg_latency << " |"
                         << std::setw(9) << as.is_finished << " |" << LOG_END;
    }
    LOG_INFO(logger)
        << "  +--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+"
        << LOG_END;
}

void Topology::log_queue_status() {
    LOG_INFO(logger) << "  +------+----------+-----------+----------+" << LOG_END;
    LOG_INFO(logger) << "  |  ID  |  Height  | Threshold | Finished |" << LOG_END;
    LOG_INFO(logger) << "  +------+----------+-----------+----------+" << LOG_END;
    for (QueueStatus &qs : get_queue_status()) {
        LOG_INFO(logger) << "  | "
                         << std::setw(4) << qs.queue_id << " | "
                         << std::setw(8) << qs.message_count << " | "
                         << std::setw(9) << qs.message_count_threshold << " | "
                         << std::setw(8) << qs.is_finished << " |" << LOG_END;
    }
    LOG_INFO(logger) << "  +------+----------+-----------+----------+" << LOG_END;
}

/// The user may want to pause the topology and interact with it manually.
/// This is particularly powerful when used from inside GDB.
StreamStatus Topology::step(const std::string &arrow_name) {
    Arrow *arrow = arrows[arrow_name];
    if (arrow == nullptr) {
        return StreamStatus::Error;
    }
    StreamStatus result = arrow->execute();
    return result;
}


} // namespace greenfield