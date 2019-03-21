#pragma once

#include <vector>
#include <mutex>

#include <greenfield/Worker.h>
#include <greenfield/Scheduler.h>
#include <greenfield/JLogger.h>


namespace greenfield {

    class ThreadManager {

        /// ThreadManager assigns a Topology to a team of Worker threads.
        /// A ThreadManager has exactly one Topology for its lifetime,
        /// which it does not own. (The caller may want to inspect the
        /// internals of the Topology, and it may be paused and re-run
        /// on a different ThreadManager.

        /// ThreadManager completely encapsulates (and thereby owns) a Worker team.

        /// ThreadManager provides the user with a high-level interface
        /// for controlling the thread team and examining its performance.
        /// The user may start, stop, kill, scale, and rebalance


    public:

        /// ThreadManager is a finite-state machine, and these are the states.
        enum class Status { Idle, Running, Stopping };

        enum class Response { Success, InProgress, NotRunning, AlreadyRunning };


    private:

        Status _status;
        std::vector<Worker*> _workers;
        Scheduler& _scheduler;


    public:

        /// A ThreadManager has exactly one Topology in its lifetime, which it does _not_ own.
        ThreadManager(Scheduler& scheduler);

        ~ThreadManager();

        /// Returns the current state of the TopologyManager
        Status get_status();

        /// The ThreadManager is idle until start() is called. run() is nonblocking.
        /// This can fail: AlreadyRunning
        Response run(int nthreads);

        /// Stop a running Topology, leaving it in a well-defined state
        /// that can be restarted and experiences no data loss. stop() is nonblocking.
        /// There are three outcomes: Success | InProgress | NotRunning
        Response stop();

        /// Block until ThreadManager is back at Status::Idle and all Worker threads have joined.
        /// This will wait until all Queues are empty and all Arrows are finished unless
        /// stop() is called first. Returns NotRunning if already joined, otherwise Success.
        Response join();

        /// Logger is public so that it can be injected by anyone who needs it
        std::shared_ptr<JLogger> logger;
    };

}


