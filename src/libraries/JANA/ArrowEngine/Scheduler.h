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

    /// Lets a Worker ask the Scheduler for another assignment. If no assignments make sense,
    /// Scheduler returns nullptr, which tells that Worker to idle until his next checkin.
    /// If next_assignment() makes any changes to internal Scheduler state or to any of its arrows,
    /// it must be synchronized.
    virtual Arrow* next_assignment(uint32_t worker_id, Arrow* assignment, JArrowMetrics::Status result) = 0;

    /// Lets a Worker tell the scheduler that he is shutting down and won't be working on his assignment
    /// any more. The scheduler is thus free to reassign the arrow to one of the remaining workers.
    virtual void last_assignment(uint32_t worker_id, Arrow* assignment, JArrowMetrics::Status result) = 0;

    /// Logger is public so that somebody else can configure it
    JLogger logger;

};

class RoundRobinScheduler : public Scheduler {
    std::vector<Arrow*> _arrows;
    size_t _next_idx = 0;
    std::mutex _mutex;
public:

    RoundRobinScheduler(Topology& topology) : _arrows(topology.get_arrows()) {}

    RoundRobinScheduler();  // Obtain topology via service locator

    Arrow* next_assignment(uint32_t worker_id, Arrow* assignment, JArrowMetrics::Status result) override;
    void last_assignment(uint32_t worker_id, Arrow* assignment, JArrowMetrics::Status result) override;

};

} // namespace arrowengine
} // namespace jana

#endif //JANA2_SCHEDULER_H
