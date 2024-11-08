
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <map>

#include "JScheduler.h"
#include <JANA/Engine/JScheduler.h>
#include <JANA/Topology/JTopologyBuilder.h>


JScheduler::JScheduler(std::shared_ptr<JTopologyBuilder> topology)
    : m_topology(topology)
    {
        m_topology_state.next_arrow_index = 0;

        // Keep track of downstream arrows
        std::map<JArrow*, size_t> arrow_map;
        size_t i=0;
        for (auto* arrow : topology->arrows) {
            arrow_map[arrow] = i++;
        }
        m_topology_state.arrow_states = std::vector<ArrowState>(topology->arrows.size());

        for (i=0; i<topology->arrows.size(); ++i) {
            auto& as = m_topology_state.arrow_states[i];
            JArrow* arrow = topology->arrows[i];
            as.arrow = arrow;
            for (JArrow* downstream : arrow->m_listeners) {
                as.downstream_arrow_indices.push_back(arrow_map[downstream]);
            }
        }
    }


JArrow* JScheduler::next_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status last_result) {

    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_TRACE(logger) << "JScheduler: Worker " << worker_id << ": Returned arrow "
                      << ((assignment == nullptr) ? "idle" : assignment->get_name()) << " -> " << to_string(last_result) << LOG_END;

    // Check latest arrow back in
    if (assignment != nullptr) {
        checkin_unprotected(assignment, last_result);
    }

    JArrow* next = checkout_unprotected();

    LOG_TRACE(logger) << "JScheduler: Worker " << worker_id << " assigned arrow "
                      << ((next == nullptr) ? "(idle)" : next->get_name()) << LOG_END;
    return next;

}


void JScheduler::last_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status last_result) {

    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_TRACE(logger) << "JScheduler: Worker " << worker_id << ": returned arrow "
                       << ((assignment == nullptr) ? "(idle)" : assignment->get_name())
                       << " -> " << to_string(last_result) << "). Shutting down!" << LOG_END;

    if (assignment != nullptr) {
        checkin_unprotected(assignment, last_result);
    }
}



void JScheduler::checkin_unprotected(JArrow* assignment, JArrowMetrics::Status last_result) {
    // Find index of arrow
    size_t index = 0;
    while (m_topology_state.arrow_states[index].arrow != assignment) index++;
    assert(index < m_topology_state.arrow_states.size());

    // Decrement arrow's thread count
    ArrowState& as = m_topology_state.arrow_states[index];
    as.thread_count -= 1;

    bool found_inactive_source = 
        assignment->is_source() && 
        last_result == JArrowMetrics::Status::Finished &&           // Only Sources get to declare themselves finished!
        as.status == ArrowStatus::Active;                           // We only want to deactivate once


    bool found_draining_stage_or_sink = 
        !assignment->is_source() &&                                 // We aren't a source
        as.active_or_draining_upstream_arrow_count == 0 &&          // All upstreams arrows are inactive
        assignment->get_pending() == 0 &&                           // All upstream queues are empty
        as.thread_count > 0 &&                                      // There are other workers still assigned to this arrow
        as.status == ArrowStatus::Active;                           // We only want to deactivate once


    bool found_inactive_stage_or_sink = 
        !assignment->is_source() &&                                 // We aren't a source
        as.active_or_draining_upstream_arrow_count == 0 &&          // All upstreams arrows are inactive
        assignment->get_pending() == 0 &&                           // All upstream queues are empty
        as.thread_count == 0 &&                                     // There are NO other workers still assigned to this arrow
        (as.status == ArrowStatus::Draining ||                      // We only want to deactivate once
            as.status == ArrowStatus::Active);


    if (found_inactive_source || found_inactive_stage_or_sink) {

        // Deactivate arrow
        // Because we are deactivating it from Active state, we know the topology has not been paused. Hence we can finalize immediately.
        assignment->finalize();
        as.status = ArrowStatus::Finalized;
        m_topology_state.active_or_draining_arrow_count--;

        LOG_TRACE(logger) << "JScheduler: Deactivated arrow " << assignment->get_name() << " (" << m_topology_state.active_or_draining_arrow_count << " remaining)" << LOG_END;

        for (size_t downstream: m_topology_state.arrow_states[index].downstream_arrow_indices) {
            m_topology_state.arrow_states[downstream].active_or_draining_upstream_arrow_count--;
        }
    }
    else if (found_draining_stage_or_sink) {
        // Drain arrow
        as.status = ArrowStatus::Draining;
        LOG_DEBUG(logger) << "JScheduler: Draining arrow " << assignment->get_name() << " (" << m_topology_state.active_or_draining_arrow_count << " remaining)" << LOG_END;
    }

    // Test if this was the last arrow running
    if (m_topology_state.active_or_draining_arrow_count == 0) {
        LOG_DEBUG(logger) << "JScheduler: All arrows are inactive. Deactivating topology." << LOG_END;
        achieve_topology_pause_unprotected();
    }
}


JArrow* JScheduler::checkout(size_t arrow_index) {
    // Note that this lets us check out Inactive arrows, whereas checkout_unprotected() does not. This because we are called by JApplicationInspector
    // whereas checkout_unprotected is called by JWorker. This is because JArrowProcessingController::request_pause shuts off the topology
    // instead of shutting off the workers, which in hindsight might have been the wrong choice.

    std::lock_guard<std::mutex> lock(m_mutex);

    if (arrow_index >= m_topology_state.arrow_states.size()) return nullptr;

    ArrowState& candidate = m_topology_state.arrow_states[arrow_index];

    if ((candidate.status == ArrowStatus::Active || candidate.status == ArrowStatus::Inactive) &&   // This excludes Draining arrows
        (candidate.arrow->is_parallel() || candidate.thread_count == 0)) {      // This excludes non-parallel arrows that are already assigned to a worker

        m_topology_state.arrow_states[arrow_index].thread_count += 1;
        return candidate.arrow;

    }
    return nullptr;
}


JArrow* JScheduler::checkout_unprotected() {

    // Choose a new arrow. Loop over all arrows, starting at where we last left off, and pick the first arrow that works
    size_t current_idx = m_topology_state.next_arrow_index;
    do {
        ArrowState& candidate = m_topology_state.arrow_states[current_idx];
        current_idx += 1;
        current_idx %= m_topology->arrows.size();

        if (candidate.status == ArrowStatus::Active &&                                // This excludes Draining arrows
            (candidate.arrow->is_parallel() || candidate.thread_count == 0)) {      // This excludes non-parallel arrows that are already assigned to a worker

                m_topology_state.next_arrow_index = current_idx; // Next time, continue right where we left off
                candidate.thread_count += 1;
                return candidate.arrow;

        }
    } while (current_idx != m_topology_state.next_arrow_index);
    return nullptr;  // We've looped through everything with no luck
}


void JScheduler::initialize_topology() {

    std::lock_guard<std::mutex> lock(m_mutex);

    assert(m_topology_state.current_topology_status == TopologyStatus::Uninitialized);
    for (JArrow* arrow : m_topology->arrows) {
        arrow->initialize();
    }
    m_topology_state.current_topology_status = TopologyStatus::Paused;
}


void JScheduler::drain_topology() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_topology_state.current_topology_status == TopologyStatus::Finalized) {
        LOG_DEBUG(logger) << "JScheduler: Draining topology: Skipping because topology is already Finalized" << LOG_END;
        return;
    }
    LOG_DEBUG(logger) << "JScheduler: Draining topology" << LOG_END;

        // We pause (as opposed to finish) for two reasons:
        // 1. There might be workers in the middle of calling eventSource->GetEvent.
        // 2. drain() might be called from a signal handler. It isn't safe to make syscalls during signal handlers
        //    due to risk of deadlock. (We technically shouldn't even do logging!)
        //
    for (size_t i=0; i<m_topology_state.arrow_states.size(); ++i) {
        ArrowState& as = m_topology_state.arrow_states[i];
        if (as.arrow->is_source()) {
            pause_arrow_unprotected(i);
        }
    }
    m_topology_state.current_topology_status = TopologyStatus::Draining;

}

void JScheduler::run_topology(int nthreads) {
    std::lock_guard<std::mutex> lock(m_mutex);
    TopologyStatus current_status = m_topology_state.current_topology_status;
    if (current_status == TopologyStatus::Running || current_status == TopologyStatus::Finalized) {
        LOG_DEBUG(logger) << "JScheduler: Running topology: " << current_status << " => " << current_status << LOG_END;
        return;
    }
    LOG_DEBUG(logger) << "JScheduler: Running topology: " << current_status << " => Running" << LOG_END;

    bool source_found = false;
    for (JArrow* arrow : m_topology->arrows) {
        if (arrow->is_source()) {
            source_found = true;
        }
    }
    if (!source_found) {
        throw JException("No event sources found!");
    }
    for (size_t i=0; i<m_topology_state.arrow_states.size(); ++i) {
        // We activate any sources, and everything downstream activates automatically
        // Note that this won't affect finished sources. It will, however, stop drain().
        ArrowState& as = m_topology_state.arrow_states[i];
        if (as.arrow->is_source()) {
            run_arrow_unprotected(i);
        }
    }
    // Note that we activate workers AFTER we activate the topology, so no actual processing will have happened
    // by this point when we start up the metrics.
    m_topology->metrics.reset();
    m_topology->metrics.start(nthreads);
    m_topology_state.current_topology_status = TopologyStatus::Running;
}

void JScheduler::request_topology_pause() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // This sets all Running arrows to Paused, which prevents Workers from picking up any additional assignments
    // Once all Workers have completed their remaining assignments, the scheduler will call achieve_pause().
    TopologyStatus current_status = m_topology_state.current_topology_status;
    if (current_status == TopologyStatus::Running) {
        LOG_DEBUG(logger) << "JScheduler: request_pause() : " << current_status << " => Pausing" << LOG_END;
        for (size_t i=0; i<m_topology_state.arrow_states.size(); ++i) {
            pause_arrow_unprotected(i);
            // If arrow is not running, pause() is a no-op
        }
        m_topology_state.current_topology_status = TopologyStatus::Pausing;
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
    TopologyStatus current_status = m_topology_state.current_topology_status;
    if (current_status == TopologyStatus::Running || current_status == TopologyStatus::Pausing || current_status == TopologyStatus::Draining) {
        LOG_DEBUG(logger) << "JScheduler: achieve_topology_pause() : " << current_status << " => " << TopologyStatus::Paused << LOG_END;
        m_topology->metrics.stop();
        m_topology_state.current_topology_status = TopologyStatus::Paused;
    }
    else {
        LOG_DEBUG(logger) << "JScheduler: achieve_topology_pause() : " << current_status << " => " << current_status << LOG_END;
    }
}

void JScheduler::finish_topology() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // This finalizes all arrows. Once this happens, we cannot restart the topology.
    // assert(m_topology_state.current_topology_status == TopologyStatus::Inactive);

    // We ensure that JFactory::EndRun() and JFactory::Finish() are called _before_ 
    // JApplication::Run() exits. This leaves the topology in an unrunnable state, 
    // but it won't be resumable anyway once it reaches Status::Finished.
    for (JEventPool* pool : m_topology->pools) {
        pool->Finalize();
    }

    // Next we finalize all remaining components (particularly JEventProcessors)
    for (ArrowState& as : m_topology_state.arrow_states) {

        if (as.status != ArrowStatus::Finalized) {
            as.arrow->finalize();
        }
    }
    m_topology_state.current_topology_status = TopologyStatus::Finalized;
}

JScheduler::TopologyStatus JScheduler::get_topology_status() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_topology_state.current_topology_status;
}

JScheduler::TopologyState JScheduler::get_topology_state() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_topology_state;
}
    

void JScheduler::run_arrow_unprotected(size_t index) {
    auto& as = m_topology_state.arrow_states[index];
    const auto& name = as.arrow->get_name();
    ArrowStatus status = as.status;

    // if (status == ArrowStatus::Unopened) {
    //     LOG_DEBUG(logger) << "Arrow '" << name << "' run(): Not initialized!" << LOG_END;
    //     throw rException("Arrow %s has not been initialized!", name.c_str());
    // }
    // if (status == ArrowStatus::Active || m_status == ArrowStatus::Finalized) {
    //     LOG_DEBUG(logger) << "Arrow '" << name << "' run() : " << status << " => " << status << LOG_END;
    //     return;
    // }
    LOG_DEBUG(logger) << "JScheduler: Activating arrow " << name << " (Previous status was " << status << ")" << LOG_END;

    m_topology_state.active_or_draining_arrow_count++;
    for (size_t downstream: m_topology_state.arrow_states[index].downstream_arrow_indices) {
        m_topology_state.arrow_states[downstream].active_or_draining_upstream_arrow_count++;
        run_arrow_unprotected(downstream); // Activating something recursively activates everything downstream.
    }
    m_topology_state.arrow_states[index].status = ArrowStatus::Active;
}

void JScheduler::pause_arrow_unprotected(size_t index) {
    auto& as = m_topology_state.arrow_states[index];
    const auto& name = as.arrow->get_name();
    ArrowStatus status = as.status;

    LOG_DEBUG(logger) << "JScheduler: Pausing arrow " << name << " (Previous status was " << status << ")" << LOG_END;
    if (status != ArrowStatus::Active) {
        return; // pause() is a no-op unless running
    }
    m_topology_state.active_or_draining_arrow_count--;
    for (size_t downstream: m_topology_state.arrow_states[index].downstream_arrow_indices) {
        m_topology_state.arrow_states[downstream].active_or_draining_upstream_arrow_count--;
    }
    m_topology_state.arrow_states[index].status = ArrowStatus::Inactive;
}

void JScheduler::finish_arrow_unprotected(size_t index) {
    auto& as = m_topology_state.arrow_states[index];
    const auto& name = as.arrow->get_name();

    ArrowStatus old_status = as.status;
    LOG_DEBUG(logger) << "JScheduler: Finishing arrow " << name << " (Previous status was " << old_status << ")" << LOG_END;
    // if (old_status == ArrowStatus::Unopened) {
    //     LOG_DEBUG(logger) << "JArrow '" << name << "': Uninitialized!" << LOG_END;
    //     throw JException("JArrow::finish(): Arrow %s has not been initialized!", name.c_str());
    // }
    if (old_status == ArrowStatus::Active) {
        m_topology_state.active_or_draining_arrow_count--;
        for (size_t downstream: m_topology_state.arrow_states[index].downstream_arrow_indices) {
            m_topology_state.arrow_states[downstream].active_or_draining_upstream_arrow_count--;
        }
    }
    if (old_status != ArrowStatus::Finalized) {
        LOG_DEBUG(logger) << "JScheduler: Finalizing arrow " << name << " (this must only happen once)" << LOG_END;
        as.arrow->finalize();
    }
    m_topology_state.arrow_states[index].status = ArrowStatus::Finalized;
}


std::ostream& operator<<(std::ostream& os, JScheduler::TopologyStatus status) {
    switch(status) {
        case JScheduler::TopologyStatus::Uninitialized: os << "Uninitialized"; break;
        case JScheduler::TopologyStatus::Running: os << "Running"; break;
        case JScheduler::TopologyStatus::Pausing: os << "Pausing"; break;
        case JScheduler::TopologyStatus::Draining: os << "Draining"; break;
        case JScheduler::TopologyStatus::Paused: os << "Paused"; break;
        case JScheduler::TopologyStatus::Finalized: os << "Finalized"; break;
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, JScheduler::ArrowStatus status) {
    switch (status) {
        case JScheduler::ArrowStatus::Uninitialized: os << "Uninitialized"; break;
        case JScheduler::ArrowStatus::Active:  os << "Active"; break;
        case JScheduler::ArrowStatus::Draining: os << "Draining"; break;
        case JScheduler::ArrowStatus::Inactive: os << "Inactive"; break;
        case JScheduler::ArrowStatus::Finalized: os << "Finalized"; break;
    }
    return os;
}


void JScheduler::summarize_arrows(std::vector<ArrowSummary>& summaries) {
    
    // Make sure we have exactly one ArrowSummary for each arrow
    if (summaries.size() != m_topology_state.arrow_states.size()) {
        summaries = std::vector<ArrowSummary>(m_topology_state.arrow_states.size());
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Copy over arrow status variables
    for (size_t i=0; i<m_topology_state.arrow_states.size(); ++i) {
        auto& as = m_topology_state.arrow_states[i];
        auto& summary = summaries[i];

        summary.arrow_name = as.arrow->get_name();
        summary.is_parallel = as.arrow->is_parallel();
        summary.is_source = as.arrow->is_source();
        summary.is_sink = as.arrow->is_sink();
        summary.messages_pending = as.arrow->get_pending();

        summary.thread_count = as.thread_count;
        summary.running_upstreams = as.active_or_draining_upstream_arrow_count;

        JArrowMetrics::Status last_status;
        size_t total_message_count;
        size_t last_message_count;
        size_t total_queue_visits;
        size_t last_queue_visits;
        JArrowMetrics::duration_t total_latency;
        JArrowMetrics::duration_t last_latency;
        JArrowMetrics::duration_t total_queue_latency;
        JArrowMetrics::duration_t last_queue_latency;

        as.arrow->get_metrics().get(
            last_status, 
            total_message_count, 
            last_message_count, 
            total_queue_visits,
            last_queue_visits, 
            total_latency, 
            last_latency, 
            total_queue_latency, 
            last_queue_latency
        );

        using millisecs = std::chrono::duration<double, std::milli>;
        auto total_latency_ms = millisecs(total_latency).count();
        auto total_queue_latency_ms = millisecs(total_queue_latency).count();

        summary.total_messages_completed = total_message_count;
        summary.last_messages_completed = last_message_count;
        summary.queue_visit_count = total_queue_visits;

        summary.avg_queue_latency_ms = (total_queue_visits == 0)
                                       ? std::numeric_limits<double>::infinity()
                                       : total_queue_latency_ms / total_queue_visits;

        summary.avg_queue_overhead_frac = total_queue_latency_ms / (total_queue_latency_ms + total_latency_ms);

        summary.avg_latency_ms = (total_message_count == 0)
                               ? std::numeric_limits<double>::infinity()
                               : total_latency_ms/total_message_count;

        summary.last_latency_ms = (last_message_count == 0)
                                ? std::numeric_limits<double>::infinity()
                                : millisecs(last_latency).count()/last_message_count;

    }


}

