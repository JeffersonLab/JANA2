
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JSCHEDULER_H_
#define _JSCHEDULER_H_

#include <mutex>
#include <vector>

#include <JANA/Topology/JArrow.h>
#include <JANA/Engine/JPerfSummary.h>
#include <JANA/Services/JLoggingService.h>


struct JArrowTopology;

/// Scheduler assigns Arrows to Workers in a first-come-first-serve manner,
/// not unlike OpenMP's `schedule dynamic`.
class JScheduler {
public:
    enum class TopologyStatus { 
        Uninitialized,   // Arrows have not been initialized() yet
        Running,         // At least one arrow is active
        Pausing,         // At least one arrow is active, but a pause has been requested
        Draining,        // At least one arrow is active, but a drain has been requested
        Paused,          // No arrows are active, but may be activated or re-activated
        Finalized        // No arrows are active, and all arrows have been finalized(), so they many not be re-activated
    };

    enum class ArrowStatus { Uninitialized,   // Arrow has not been initialized() yet
                             Active,          // Arrow may be scheduled
                             Draining,        // All upstreams are inactive and queues are emptied, but arrow still has at least one worker out. Should not be scheduled.
                             Inactive,        // Arrow should not be scheduled, but may be re-activated
                             Finalized        // Arrow should not be scheduled, and has been finalized(), so it may not be re-activated
                            };

    struct ArrowState {
        JArrow* arrow = nullptr;
        ArrowStatus status = ArrowStatus::Uninitialized;
        int64_t thread_count = 0;                  // Current number of threads assigned to this arrow
        int64_t active_or_draining_upstream_arrow_count = 0;        // Current number of active or draining arrows immediately upstream
        std::vector<size_t> downstream_arrow_indices; 
    };

    struct TopologyState {
        std::vector<ArrowState> arrow_states;
        TopologyStatus current_topology_status = TopologyStatus::Uninitialized;
        int64_t active_or_draining_arrow_count = 0;  // Detects when the topology has paused
        size_t next_arrow_index;
    };

private:
    // This mutex controls ALL scheduler state
    std::mutex m_mutex;

    std::shared_ptr<JArrowTopology> m_topology;

    // Protected state
    TopologyState m_topology_state;


public:

    /// Constructor. Note that a Scheduler operates on a vector of Arrow*s.
    JScheduler(std::shared_ptr<JArrowTopology> topology);


    // Worker-facing operations
    
    /// Lets a Worker ask the Scheduler for another assignment. If no assignments make sense,
    /// Scheduler returns nullptr, which tells that Worker to idle until his next checkin.
    /// If next_assignment() makes any changes to internal Scheduler state or to any of its arrows,
    /// it must be synchronized.
    JArrow* next_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status result);

    /// Lets a Worker tell the scheduler that he is shutting down and won't be working on his assignment
    /// any more. The scheduler is thus free to reassign the arrow to one of the remaining workers.
    void last_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status result);

    /// Logger is public so that somebody else can configure it
    JLogger logger;


    void initialize_topology();
    void drain_topology();
    void run_topology(int nthreads);
    void request_topology_pause();
    void achieve_topology_pause();
    void finish_topology();

    TopologyStatus get_topology_status(); 
    TopologyState get_topology_state();
    void summarize_arrows(std::vector<ArrowSummary>& summaries);


private:
    void achieve_topology_pause_unprotected();
    void run_arrow_unprotected(size_t index);
    void pause_arrow_unprotected(size_t index);
    void finish_arrow_unprotected(size_t index);
    void checkin_unprotected(JArrow* arrow, JArrowMetrics::Status last_result);
    JArrow* checkout_unprotected();

};

std::ostream& operator<<(std::ostream& os, JScheduler::TopologyStatus status);
std::ostream& operator<<(std::ostream& os, JScheduler::ArrowStatus status);

#endif // _JSCHEDULER_H_


