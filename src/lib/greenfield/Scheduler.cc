
#include <greenfield/Scheduler.h>

namespace greenfield {


Scheduler::Scheduler(const std::vector<Arrow*>& arrows, size_t worker_count)
    : _arrows(arrows)
    , _worker_count(worker_count)
    , _next_idx(0)
    {
        _logger = LoggingService::logger("Scheduler");
    }


Arrow* Scheduler::next_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus last_result) {

    // Short-circuit our scheduler visit if everything is in order.
    if (last_result == StreamStatus::KeepGoing && assignment != nullptr) {
        return assignment;
    }

    _mutex.lock();


    // Check latest arrow back in
    if (assignment != nullptr) {
        assignment->update_thread_count(-1);

        if (assignment->is_upstream_finished() && assignment->get_thread_count() == 0) {

            // This was the last worker running this arrow, so it can now deactivate
            // We know it is the last worker because we stopped assigning them
            // once is_upstream_finished started returning true
            assignment->set_active(false);

            _mutex.unlock();
            LOG_INFO(_logger) << "Scheduler: Deactivating arrow " << assignment->get_name() << LOG_END;
            assignment->notify_downstream(false);
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

        if (!candidate->is_upstream_finished() &&
            (candidate->is_parallel() || candidate->get_thread_count() == 0)) {

            // Found a plausible candidate; done
            _next_idx = current_idx; // Next time, continue right where we left off
            candidate->update_thread_count(1);

            LOG_DEBUG(_logger) << "Scheduler: (" << worker_id << ", "
                              << ((assignment == nullptr) ? "idle" : assignment->get_name())
                              << ", " << to_string(last_result) << ") => "
                              << candidate->get_name() << "  [" << candidate->get_thread_count() << "]" << LOG_END;
            _mutex.unlock();
            return candidate;
        }

    } while (current_idx != _next_idx);

    _mutex.unlock();
    return nullptr;  // We've looped through everything with no luck
}


void Scheduler::last_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus result) {

    _worker_count--;
    if (assignment != nullptr) {
        assignment->update_thread_count(-1);
    }
}


} // namespace greenfield


