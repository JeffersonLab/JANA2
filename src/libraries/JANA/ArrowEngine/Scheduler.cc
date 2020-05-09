
#include <JANA/ArrowEngine/Scheduler.h>

namespace jana {
namespace arrowengine {




Arrow* RoundRobinScheduler::next_assignment(uint32_t worker_id, Arrow* assignment, JArrowMetrics::Status last_result) {

    _mutex.lock();

    // Check latest arrow back in
    if (assignment != nullptr) {
        assignment->thread_count--;

        /// Just because one worker thinks the arrow is finished doesn't mean that it is safe for downstream
        /// to assume that all workers running that arrow are finished. Thus it is up to the scheduler to
        /// declare when an arrow is finished based on looking at all of the workers
        if (last_result==JArrowMetrics::Status::Finished && assignment->thread_count == 0) {

            // This was the last worker running this arrow, so it can now deactivate
            // We know it is the last worker because we stopped assigning them
            // once is_upstream_finished started returning true
            assignment->is_finished = true;
            for (auto downstream : assignment->downstreams) {
                downstream->active_upstream_count -= 1;
            }

            LOG_INFO(logger) << "Deactivating arrow " << assignment->name << LOG_END;
            _mutex.lock();
        }
    }

    // Choose a new arrow. Loop over all arrows, starting at where we last left off, and pick the first
    // arrow that works
    size_t current_idx = _next_idx;
    do {
        Arrow* candidate = _arrows[current_idx];
        current_idx += 1;
        current_idx %= _arrows.size();

        if (!candidate->is_finished && (candidate->is_parallel || candidate->thread_count == 0)) {

            // Found a plausible candidate; done
            _next_idx = current_idx; // Next time, continue right where we left off
            candidate->thread_count += 1;

            LOG_DEBUG(logger) << "Worker " << worker_id << ", "
                              << ((assignment == nullptr) ? "idle" : assignment->name)
                              << ", " << to_string(last_result) << " => "
                              << candidate->name << "  [" << candidate->thread_count.load() << "]" << LOG_END;
            _mutex.unlock();
            return candidate;
        }

    } while (current_idx != _next_idx);

    _mutex.unlock();
    return nullptr;  // We've looped through everything with no luck. Consider telling JAPC to shut down timer, topology to shut down
}


void RoundRobinScheduler::last_assignment(uint32_t worker_id, Arrow* assignment, JArrowMetrics::Status result) {

    LOG_DEBUG(logger) << "Worker " << worker_id << ", "
                      << ((assignment == nullptr) ? "idle" : assignment->name)
                      << ", " << to_string(result) << ") => Shutting down!" << LOG_END;
    _mutex.lock();
    if (assignment != nullptr) {
        assignment->thread_count--;
    }
    _mutex.unlock();
}

}
}


