
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JSCHEDULER_H_
#define _JSCHEDULER_H_

#include <mutex>

#include <JANA/Engine/JArrow.h>
#include <JANA/Services/JLoggingService.h>


    struct JArrowTopology;

    /// Scheduler assigns Arrows to Workers in a first-come-first-serve manner,
    /// not unlike OpenMP's `schedule dynamic`.
    class JScheduler {

    private:
        std::shared_ptr<JArrowTopology> m_topology;
        size_t m_next_idx;
        std::mutex m_mutex;

    public:

        /// Constructor. Note that a Scheduler operates on a vector of Arrow*s.
        JScheduler(std::shared_ptr<JArrowTopology> topology);

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
    };


#endif // _JSCHEDULER_H_


