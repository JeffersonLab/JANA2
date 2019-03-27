

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

        if (report.last_result != StreamStatus::KeepGoing) {

            if (next != nullptr) { next->update_thread_count(-1); }
            next = next_assignment();
            if (next != nullptr) { next->update_thread_count(1); }
        }
        _mutex.unlock();
        LOG_DEBUG(logger) << "Scheduler: (" << report.worker_id << ", "
                     << ((report.assignment == nullptr) ? "nullptr" : report.assignment->get_name())
                     << ", " << to_string(report.last_result) << ") => "
                     << ((next == nullptr) ? "nullptr" : next->get_name())
                     << "  [" << next->get_thread_count() << "]" << LOG_END;
        return next;

    }


    Arrow* RoundRobinScheduler::next_assignment() {

        while (_assignments.size() > 0) {
            Arrow *arrow = _assignments.front();
            _assignments.pop();
            if (!arrow->is_active()) {
                // Removes arrow from the queue completely and tries the next one
                continue;
            }
            else if (!arrow->is_parallel() && arrow->get_thread_count() != 0) {
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


