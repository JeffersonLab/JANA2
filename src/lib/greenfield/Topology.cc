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

int Topology::next_index() {
    return arrows.size();
}

void Topology::addManagedComponent(Component *component) {
    components.push_back(component);
}

void Topology::addQueue(QueueBase *queue) {
    queue->set_id(queues.size());
    queues.push_back(queue);
}

void Topology::addArrow(Arrow *arrow) {
    // Note that arrow name lives on the Topology, not on the Arrow
    // itself: different instances of the same Arrow can
    // be assigned different places in the same Topology, but
    // the users are inevitably going to make the Arrow
    // name be constant w.r.t to the Arrow class unless we force
    // them to do it correctly.

    // TODO: Commenting out these lines is going to cause problems
    //arrow->set_name(name);
    //arrow->set_id(arrows.size());

    arrows[arrow->get_name()] = arrow;
    _arrow_statuses.emplace_back();
    ArrowStatus &status = _arrow_statuses.back();

    status.arrow_name = arrow->get_name();
    status.arrow_id = arrow->get_index();
    status.is_parallel = arrow->is_parallel();
    status.is_finished = !arrow->is_active();
    status.short_term_avg_latency = 0;
    status.long_term_avg_latency = 0;
    status.thread_count = 0;
    status.messages_completed = 0;
};

void Topology::finalize() {
}

void Topology::activate(std::string arrow_name) {
    arrows[arrow_name]->set_active(true);
}

void Topology::deactivate(std::string arrow_name) {
    arrows[arrow_name]->set_active(false);
}

void Topology::report_arrow_finished(Arrow *arrow) {

    LOG_DEBUG(logger) << "Arrow reported finished: " << arrow->get_name() << LOG_END;
}

void Topology::update(Arrow *arrow, StreamStatus last_result, double latency, uint64_t messages_completed) {

    ArrowStatus &arrowStatus = _arrow_statuses[arrow->get_index()];
    arrowStatus.messages_completed += messages_completed;
    arrowStatus.long_term_avg_latency += latency;
    arrowStatus.short_term_avg_latency = latency / messages_completed;
    if (last_result == StreamStatus::Finished && arrowStatus.thread_count == 0) {
        report_arrow_finished(arrow);
    }
}

std::vector<Topology::ArrowStatus> Topology::get_arrow_status() {

    std::vector<ArrowStatus> metrics;

    for (auto pair : arrows) {
        Arrow* arrow = pair.second;
        ArrowStatus status;
        status.arrow_name = arrow->get_name();
        status.arrow_id = arrow->get_index();
        status.is_finished = !arrow->is_active();
        status.thread_count = arrow->get_thread_count();
        status.messages_completed = arrow->get_message_count();
        status.is_parallel = arrow->is_parallel();
        // TODO: Add latency and overhead calculations back
        metrics.push_back(status);

    }
    for (auto &metric : metrics) {
        // Convert from running total to average
        metric.long_term_avg_latency /= metric.messages_completed;
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
        << "  +------+--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+"
        << LOG_END;
    LOG_INFO(logger)
        << "  |  ID  |              Name              | Parallel | Threads | Messages completed | Latency (short) | Latency (long) | Finished |"
        << LOG_END;
    LOG_INFO(logger)
        << "  +------+--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+"
        << LOG_END;
    for (ArrowStatus &as : get_arrow_status()) {
        LOG_INFO(logger) << "  | "
                         << std::setw(4) << as.arrow_id << " | "
                         << std::setw(30) << std::left << as.arrow_name << " | "
                         << std::setw(8) << std::right << as.is_parallel << " | "
                         << std::setw(7) << as.thread_count << " |"
                         << std::setw(19) << as.messages_completed << " |"
                         << std::setw(16) << as.short_term_avg_latency << " |"
                         << std::setw(15) << as.long_term_avg_latency << " |"
                         << std::setw(9) << as.is_finished << " |" << LOG_END;
    }
    LOG_INFO(logger)
        << "  +------+--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+"
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
    if (result == StreamStatus::Finished && _arrow_statuses[arrow->get_index()].thread_count == 0) {
        report_arrow_finished(arrow);
    }
    return result;
}


} // namespace greenfield