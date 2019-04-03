//
// Created by nbrei on 3/29/19.
//

#include <greenfield/Scheduler.h>

namespace greenfield {

thread_local size_t IndependentScheduler::next_idx = 0;


IndependentScheduler::IndependentScheduler(Topology& topology) : _topology(topology) {}


Arrow* IndependentScheduler::next_assignment (uint32_t worker_id, Arrow* assignment, StreamStatus last_result) {

    if (last_result == StreamStatus::KeepGoing) {
        return assignment;
    }
    size_t current = next_idx;
    do {
        Arrow *candidate = _topology.arrows[next_idx];
        current += 1;
        current %= _topology.arrows.size();

        if (candidate->is_active() &&
            (candidate->is_parallel() || candidate->get_thread_count() == 0)) {

            // Found a plausible candidate; done
            next_idx = current; // Next time, continue right where we left off
            if (candidate != assignment) {
                if (assignment != nullptr) {
                    assignment->update_thread_count(-1);
                }
                candidate->update_thread_count(1);
            }

            LOG_DEBUG(logger) << "IndependentScheduler: (" << worker_id << ", "
                              << ((assignment == nullptr) ? "idle" : assignment->get_name())
                              << ", " << to_string(last_result) << ") => "
                              << candidate->get_name() << "  [" << candidate->get_thread_count() << "]" << LOG_END;
            return candidate;
        }

    } while (next_idx != current);

    // No candidates found; idle for a while
    LOG_DEBUG(logger) << "IndependentScheduler: (" << worker_id << ", "
                      << ((assignment == nullptr) ? "idle" : assignment->get_name())
                      << ", " << to_string(last_result) << ") => " << "idle" << LOG_END;
    return nullptr;

}

}
