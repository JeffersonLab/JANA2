

#include <greenfield/Scheduler.h>
#include <greenfield/Logger.h>

namespace greenfield {


    RoundRobinScheduler::RoundRobinScheduler(Topology &topology)
        : _topology(topology)
        , _assignment(_topology.arrows.begin())
    {
        logger = LoggingService::logger("Scheduler");
    }


    Arrow* RoundRobinScheduler::next_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus last_result) {

        Arrow* next = assignment;

        if (last_result != StreamStatus::KeepGoing) {
            // We need a new assignment

            _mutex.lock();
            if (next != nullptr) {
                next->update_thread_count(-1);

                if (next->is_upstream_finished() && next->get_thread_count()==0) {
                    // This was the last thread, arrow can now deactivate
                    next->set_active(false);
                    next->notify_downstream(false);
                    LOG_INFO(logger) << "Scheduler: Deactivating arrow " << next->get_name() << LOG_END;
                }
            }
            next = next_assignment();
            if (next != nullptr) { next->update_thread_count(1); }
            _mutex.unlock();
        }
        LOG_DEBUG(logger) << "Scheduler: (" << worker_id << ", "
                     << ((assignment == nullptr) ? "idle" : assignment->get_name())
                     << ", " << to_string(last_result) << ") => "
                     << ((next == nullptr) ? "idle" : next->get_name())
                     << "  [" << ((next == nullptr) ? 0 : next->get_thread_count()) << "]" << LOG_END;
        return next;

    }


    Arrow* RoundRobinScheduler::next_assignment() {

        auto next = _assignment;
        do {
            Arrow *candidate = *next;
            //LOG_TRACE(logger) << "RRS: Examining candidate " << candidate->get_name() << LOG_END;

            ++next; // Move the iterator forward
            if (next == _topology.arrows.end()) {  // Reached end of container
                next = _topology.arrows.begin(); // Start again at the beginning
            }

            if (!candidate->is_upstream_finished() &&
                (candidate->is_parallel() || candidate->get_thread_count() == 0)) {

                // Found a plausible candidate; done
                _assignment = next; // Next time, continue right where we left off
                return candidate;
            }

        } while (next != _assignment);  // We have looped over everything and found nothing

        // Since there were no plausible candidates, return a nullptr, which tells Worker to shut down
        return nullptr;
    }

}


