//
// Created by Nathan Brei on 4/29/20.
//

#ifndef JANA2_SCHEDULER_H
#define JANA2_SCHEDULER_H

#include <JANA/ArrowEngine/Arrow.h>
#include <JANA/ArrowEngine/Topology.h>
#include <JANA/JLogger.h>

#include <mutex>

namespace jana {
namespace arrowengine {

struct Scheduler : public JService {

    virtual JArrow* next_assignment(uint32_t worker_id, Arrow* assignment, JArrowMetrics::Status result);

    /// Lets a Worker ask the Scheduler for another assignment. If no assignments make sense,
    /// Scheduler returns nullptr, which tells that Worker to idle until his next checkin.
    /// If next_assignment() makes any changes to internal Scheduler state or to any of its arrows,
    /// it must be synchronized.
    virtual JArrow* next_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status result);

    /// Lets a Worker tell the scheduler that he is shutting down and won't be working on his assignment
    /// any more. The scheduler is thus free to reassign the arrow to one of the remaining workers.
    virtual void last_assignment(uint32_t worker_id, JArrow* assignment, JArrowMetrics::Status result);

    /// Logger is public so that somebody else can configure it
    JLogger logger;

};

class RoundRobinScheduler : public Scheduler {
    const std::vector<Arrow*> _arrows;
    size_t _next_idx;
    std::mutex _mutex;
public:
    RoundRobinScheduler(Topology& topology);   // Obtain topology via dep inj
    RoundRobinScheduler();  // Obtain topology via service locator

};

} // namespace arrowengine
} // namespace jana

#endif //JANA2_SCHEDULER_H
