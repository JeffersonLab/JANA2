

#include <greenfield/Scheduler.h>
#include <greenfield/JLogger.h>

namespace greenfield {


    RoundRobinScheduler::RoundRobinScheduler(Topology &topology) :
        _topology(topology) {

        for (auto &pair : topology.arrows) {
            _assignments.push(pair.second);
        }
    }


    Arrow* RoundRobinScheduler::next_assignment(const greenfield::Scheduler::Report &report) {

        _mutex.lock();
        Arrow* next = report.assignment;
        if (next != nullptr) {
            _topology.update(report.assignment, report.last_result, report.latency_sum, report.event_count);
        }

        if (report.last_result != StreamStatus::KeepGoing) {

            if (next != nullptr) { _topology._arrow_statuses[next->get_index()].thread_count--; }
            next = next_assignment();
            if (next != nullptr) { _topology._arrow_statuses[next->get_index()].thread_count++; }
        }
        _mutex.unlock();
        LOG_DEBUG(logger) << "Scheduler: (" << report.worker_id << ", "
                     << ((report.assignment == nullptr) ? "nullptr" : report.assignment->get_name())
                     << ", " << to_string(report.last_result) << ") => "
                     << ((next == nullptr) ? "nullptr" : next->get_name())
                     << "  [" << _topology._arrow_statuses[next->get_index()].thread_count << "]" << LOG_END;
        return next;

    }

    // TODO: Three different places where we track arrow->is_finished. Which do we really want?
    // - Arrow::is_finished                   ---- This is most definitive, and consistent with Queue.is_finished
    // - We know a queue is finished when the downstream arrow is also finished.
    // - Scheduler::_metrics::is_finished     ---- This is only supposed to be a view
    // - StreamStatus::Finished              ---- This is kind of redundant. execute() could return bool
    //                                        ---- On the other hand, this way we don't have to sync Arrows or queues
    //                                        ---- But: who tells a Queue it is finished?


    Arrow* RoundRobinScheduler::next_assignment() {

        while (_assignments.size() > 0) {
            Arrow *arrow = _assignments.front();
            _assignments.pop();
            if (arrow->is_finished()) {
                // Removes arrow from the queue completely and tries the next one
                continue;
            }
            else if (!arrow->is_parallel() && _topology._arrow_statuses[arrow->get_index()].thread_count != 0) {
                // Arrow is sequential and is already assigned to a thread,
                // so we send it to the back of the queue and try again
                _assignments.push(arrow);
                continue;
            }
            else {
                // Arrow is either (parallel) or (sequential and not assigned to any threads)
                // We send it to the back of the queue and also return it
                _assignments.push(arrow);
                return arrow;
            }
        }
        return nullptr;  // null represents 'no viable arrows found'
    }
}


