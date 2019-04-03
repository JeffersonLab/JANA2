#pragma once

#include <map>
#include <queue>
#include <mutex>

#include <greenfield/Topology.h>
#include "Logger.h"


namespace greenfield {

    class Scheduler {

    public:
        /// Lets a Worker ask the Scheduler for another assignment. If no assignments make sense,
        /// Scheduler returns nullptr, which tells that Worker to idle until his next checkin.
        /// If next_assignment() makes any changes to internal Scheduler state or to any of its arrows,
        /// it must be synchronized.
        virtual Arrow *next_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus result) = 0;

        /// Lets a Worker tell the scheduler that he is shutting down and won't be working on his assignment
        /// any more. The scheduler is thus free to reassign the arrow to one of the remaining workers.
        virtual void last_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus result) {
            if (assignment != nullptr) {
                assignment->update_thread_count(-1);
            }
        }

        Logger logger; // Control verbosity of scheduler without knowing which impl is in use
    };



    class FixedScheduler : public Scheduler {
        /// FixedScheduler assumes the user knows how to map Arrows to Workers, and doesn't diverge from this
        /// even when the Arrows return ComeBackLater. This works well when the latency of each Arrow has a narrow
        /// spread.
        ///
        /// If the Arrow latencies are known in advance, it is easy to calculate how many Workers each Arrow needs
        /// in order to obtain the highest overall throughput:
        ///
        /// 0. For all sequential arrows a, let nworkers(a) := 1
        /// 1. Let latency_max := max(latency(a)) for all sequential arrows a
        /// 2. For all parallel arrows a, let nworkers(a) := ceil(latency(a) / latency_max)
        ///
        /// Its tradeoffs are:
        ///
        /// 1. Pro: Extremely low overhead
        /// 2. Pro: Supports interactive real-time manual rebalancing
        /// 3. Pro: No opportunity to get caught in a livelock state
        /// 4. Con: If the latency of any Arrow has too large a spread, the theoretical optimum
        ///         throughput might not be reached
        /// 5. Con: If the estimated latency of an Arrow differs too much from the
        ///         average latency, the height of some queues may grow without bound

    private:
        Topology& _topology;
        std::vector<Arrow*> _assignments;

    public:
        FixedScheduler(Topology& topology, std::map<std::string, int> nthreads);
        bool rebalance(std::string, std::string, int);
        Arrow* next_assignment(uint32_t, Arrow*, StreamStatus) override;
    };



    class RoundRobinScheduler : public Scheduler {
        /// RoundRobinScheduler assigns Arrows to Workers in a first-come-first-serve manner,
        /// not unlike OpenMP's `schedule dynamic`. Its tradeoffs are:

        /// 1. Pro: It can load-balance Arrows that have unpredictable latency
        /// 2. Pro: It won't get caught in weird livelock states
        /// 3. Con: It adds a synchronization overhead

    private:
        Topology& _topology;
        std::vector<Arrow*>::iterator _assignment;
        std::mutex _mutex;

        Arrow* next_assignment();

    public:
        explicit RoundRobinScheduler(Topology& topology);
        Arrow* next_assignment(uint32_t, Arrow*, StreamStatus) override;
    };

    class IndependentScheduler : public Scheduler {
        /// IndependentScheduler has similar semantics to what JANA2 was doing before, where each
        /// JThread polls each Queue round-robin on its own. It differs from RoundRobinScheduler
        /// in two important ways:
        ///
        /// 1. The call to next_assignment has no synchronization, making this scheduler particularly low-overhead
        /// 2. Each worker starts by looking at same arrow. Assuming the arrows are ordered upstream-to-downstream:
        ///
        ///    a. Initially, the SourceArrow will be subscribed once, the first MapArrow will be
        ///       maximally subscribed, and downstream MapArrows and SinkArrows won't be subscribed at all
        ///    b. The downstream MapArrows and SinkArrows will start being subscribed only once the upstream
        ///       _output_ queues have all filled up. Hence the queue threshold size becomes important.
        ///
        /// The tradeoffs are:
        ///
        /// 1. Pro: Low overhead due to lack of synchronization
        /// 2. Pro: It can load-balance Arrows that have unpredictable latency
        /// 3. Con: Currently unclear if this implementation is free of livelock

    private:
        Topology& _topology;
        static thread_local size_t next_idx;

    public:
        IndependentScheduler(Topology& topology);
        Arrow* next_assignment(uint32_t, Arrow*, StreamStatus) override;
    };

}




