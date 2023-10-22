
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Engine/JScheduler.h>
#include <JANA/Engine/JArrowTopology.h>
#include <JANA/Services/JLoggingService.h>


JScheduler::JScheduler(std::shared_ptr<JArrowTopology> topology)
    : m_topology(topology)
    , m_next_idx(0)
    {
    }


JArrow* JScheduler::next_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status last_result) {

    m_mutex.lock();

    // Check latest arrow back in
    if (assignment != nullptr) {
        assignment->update_thread_count(-1);

        if (assignment->get_running_upstreams() == 0 &&
            assignment->get_pending() == 0 &&
            assignment->get_thread_count() == 0 &&
            assignment->get_type() != JArrow::NodeType::Source &&
                assignment->get_status() == JArrow::Status::Running) {

            LOG_DEBUG(logger) << "Deactivating arrow '" << assignment->get_name() << "' (" << running_arrow_count - 1 << " remaining)" << LOG_END;
            assignment->pause();
            assert(running_arrow_count >= 0);
            if (running_arrow_count == 0) {
                LOG_DEBUG(logger) << "All arrows deactivated. Deactivating topology." << LOG_END;
                achieve_topology_pause_unprotected();
            }
        }
    }

    // Choose a new arrow. Loop over all arrows, starting at where we last left off, and pick the first
    // arrow that works
    size_t current_idx = m_next_idx;
    do {
        JArrow* candidate = m_topology->arrows[current_idx];
        current_idx += 1;
        current_idx %= m_topology->arrows.size();

        if (candidate->get_status() == JArrow::Status::Running &&
            (candidate->is_parallel() || candidate->get_thread_count() == 0)) {

            // Found a plausible candidate.

            if (candidate->get_type() == JArrow::NodeType::Source ||
                candidate->get_running_upstreams() > 0 ||
                candidate->get_pending() > 0) {

                // Candidate still has work they can do
                m_next_idx = current_idx; // Next time, continue right where we left off
                candidate->update_thread_count(1);

                LOG_DEBUG(logger) << "Worker " << worker_id << ", "
                                  << ((assignment == nullptr) ? "idle" : assignment->get_name())
                                  << ", " << to_string(last_result) << " => "
                                  << candidate->get_name() << "  [" << candidate->get_thread_count() << " threads]" << LOG_END;
                m_mutex.unlock();
                return candidate;

            }
            else {
                // Candidate can be paused immediately because there is no more work coming
                LOG_DEBUG(logger) << "Deactivating arrow '" << candidate->get_name() << "' (" << running_arrow_count - 1 << " remaining)" << LOG_END;
                candidate->pause();
                assert(running_arrow_count >= 0);
                if (running_arrow_count == 0) {
                    LOG_DEBUG(logger) << "All arrows deactivated. Deactivating topology." << LOG_END;
                    achieve_topology_pause_unprotected();
                }
            }
        }

    } while (current_idx != m_next_idx);

    if (running_arrow_count == 0 && m_current_topology_status == TopologyStatus::Running) {
        // This exists just in case the user provided a topology that cannot self-exit, e.g. because no event sources
        // have been specified. If no arrows can be assigned to this worker, and no arrows have yet been assigned to any
        // workers, we can conclude that the topology is dead. After deactivating the topology, is_stopped() returns true,
        // and Run(true) terminates.
        LOG_DEBUG(logger) << "No active arrows found. Deactivating topology." << LOG_END;
        achieve_topology_pause_unprotected();
    }
    m_mutex.unlock();

    return nullptr;  // We've looped through everything with no luck
}


void JScheduler::last_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status result) {

    LOG_DEBUG(logger) << "Worker " << worker_id << ", "
                       << ((assignment == nullptr) ? "idle" : assignment->get_name())
                       << ", " << to_string(result) << ") => Shutting down!" << LOG_END;
    m_mutex.lock();
    if (assignment != nullptr) {
        assignment->update_thread_count(-1);
    }
    m_mutex.unlock();
}


void JScheduler::initialize_topology() {

    std::lock_guard<std::mutex> lock(m_mutex);

    assert(m_current_topology_status == TopologyStatus::Uninitialized);
    for (JArrow* arrow : m_topology->arrows) {
        arrow->initialize();
    }
    m_current_topology_status = TopologyStatus::Paused;
}


void JScheduler::drain_topology() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_current_topology_status == TopologyStatus::Finished) {
        LOG_DEBUG(logger) << "JScheduler: drain(): Skipping because topology is already Finished" << LOG_END;
        return;
    }
    LOG_DEBUG(logger) << "JScheduler: drain_topology()" << LOG_END;
    for (auto source : m_topology->sources) {
        source->pause();
        m_current_topology_status = TopologyStatus::Draining;
        // We pause (as opposed to finish) for two reasons:
        // 1. There might be workers in the middle of calling eventSource->GetEvent.
        // 2. drain() might be called from a signal handler. It isn't safe to make syscalls during signal handlers
        //    due to risk of deadlock. (We technically shouldn't even do logging!)
    }

}

void JScheduler::run_topology(int nthreads) {
    std::lock_guard<std::mutex> lock(m_mutex);
    TopologyStatus current_status = m_current_topology_status;
    if (current_status == TopologyStatus::Running || current_status == TopologyStatus::Finished) {
        LOG_DEBUG(logger) << "JScheduler: run_topology() : " << current_status << " => " << current_status << LOG_END;
        return;
    }
    LOG_DEBUG(logger) << "JScheduler: run_topology() : " << current_status << " => Running" << LOG_END;

    for (auto arrow : m_topology->arrows) {
        arrow->set_running_arrows(&running_arrow_count);
    }

    if (m_topology->sources.empty()) {
        throw JException("No event sources found!");
    }
    for (auto source : m_topology->sources) {
        // We activate any sources, and everything downstream activates automatically
        // Note that this won't affect finished sources. It will, however, stop drain().
        source->run();
    }
    // Note that we activate workers AFTER we activate the topology, so no actual processing will have happened
    // by this point when we start up the metrics.
    m_topology->metrics.reset();
    m_topology->metrics.start(nthreads);
    m_current_topology_status = TopologyStatus::Running;
}

void JScheduler::request_topology_pause() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // This sets all Running arrows to Paused, which prevents Workers from picking up any additional assignments
    // Once all Workers have completed their remaining assignments, the scheduler will call achieve_pause().
    TopologyStatus current_status = m_current_topology_status;
    if (current_status == TopologyStatus::Running) {
        LOG_DEBUG(logger) << "JScheduler: request_pause() : " << current_status << " => Pausing" << LOG_END;
        for (auto arrow: m_topology->arrows) {
            arrow->pause();
            // If arrow is not running, pause() is a no-op
        }
        m_current_topology_status = TopologyStatus::Pausing;
    }
    else {
        LOG_DEBUG(logger) << "JScheduler: request_pause() : " << current_status << " => " << current_status << LOG_END;
    }
}

void JScheduler::achieve_topology_pause() {

    std::lock_guard<std::mutex> lock(m_mutex);
    achieve_topology_pause_unprotected();
}

void JScheduler::achieve_topology_pause_unprotected() {

    // This is meant to be used by the scheduler to tell us when all workers have stopped, so it is safe to finish(), etc
    TopologyStatus current_status = m_current_topology_status;
    if (current_status == TopologyStatus::Running || current_status == TopologyStatus::Pausing || current_status == TopologyStatus::Draining) {
        LOG_DEBUG(logger) << "JScheduler: achieve_topology_pause() : " << current_status << " => " << TopologyStatus::Paused << LOG_END;
        m_topology->metrics.stop();
        m_current_topology_status = TopologyStatus::Paused;
    }
    else {
        LOG_DEBUG(logger) << "JScheduler: achieve_topology_pause() : " << current_status << " => " << current_status << LOG_END;
    }
}

void JScheduler::finish_topology() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // This finalizes all arrows. Once this happens, we cannot restart the topology.
    // assert(m_current_topology_status == TopologyStatus::Paused);
    for (auto arrow : m_topology->arrows) {
        arrow->finish();
    }
    m_current_topology_status = TopologyStatus::Finished;
}

JScheduler::TopologyStatus JScheduler::get_topology_status() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_topology_status;
}
    


std::ostream& operator<<(std::ostream& os, JScheduler::TopologyStatus status) {
    switch(status) {
        case JScheduler::TopologyStatus::Uninitialized: os << "Uninitialized"; break;
        case JScheduler::TopologyStatus::Running: os << "Running"; break;
        case JScheduler::TopologyStatus::Pausing: os << "Pausing"; break;
        case JScheduler::TopologyStatus::Paused: os << "Paused"; break;
        case JScheduler::TopologyStatus::Finished: os << "Finished"; break;
        case JScheduler::TopologyStatus::Draining: os << "Draining"; break;
    }
    return os;
}
