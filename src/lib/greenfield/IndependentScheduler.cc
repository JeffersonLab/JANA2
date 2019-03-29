//
// Created by nbrei on 3/29/19.
//

#include <greenfield/Scheduler.h>

namespace greenfield {

thread_local size_t IndependentScheduler::next_idx = 0;


IndependentScheduler::IndependentScheduler(Topology& topology) : _topology(topology) {}


Arrow* IndependentScheduler::next_assignment (const Report& report) {

    if (report.last_result == StreamStatus::KeepGoing) {
        return report.assignment;
    }
    size_t current = next_idx;
    do {
        // TODO: Fix this after changing topology.arrows to vector
        auto it = _topology.arrows.begin();
        for (size_t i=0; i<next_idx; ++i) {
            it++;
        }
        Arrow *candidate = it->second;
        current += 1;
        current %= _topology.arrows.size();

        if (candidate->is_active() &&
            (candidate->is_parallel() || candidate->get_thread_count() == 0)) {

            // Found a plausible candidate; done
            next_idx = current; // Next time, continue right where we left off
            if (candidate != report.assignment) {
                if (report.assignment != nullptr) {
                    report.assignment->update_thread_count(-1);
                }
                candidate->update_thread_count(1);
            }

            LOG_DEBUG(logger) << "IndependentScheduler: (" << report.worker_id << ", "
                              << ((report.assignment == nullptr) ? "idle" : report.assignment->get_name())
                              << ", " << to_string(report.last_result) << ") => "
                              << candidate->get_name() << "  [" << candidate->get_thread_count() << "]" << LOG_END;
            return candidate;
        }

    } while (next_idx != current);

    // No candidates found; idle for a while
    LOG_DEBUG(logger) << "IndependentScheduler: (" << report.worker_id << ", "
                      << ((report.assignment == nullptr) ? "idle" : report.assignment->get_name())
                      << ", " << to_string(report.last_result) << ") => " << "idle" << LOG_END;
    return nullptr;

}

}
