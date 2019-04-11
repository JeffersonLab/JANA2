#pragma once

#include <vector>
#include <mutex>

#include <JANA/JWorker.h>
#include <JANA/JScheduler.h>
#include <JANA/JLogger.h>


class JThreadTeam {

    /// JThreadTeam completely encapsulates (and thereby owns) a team of JWorkers.

    /// JThreadTeam provides a high-level interface
    /// for controlling the thread team and examining its performance.
    /// The user may start, stop, kill, scale, and rebalance


public:

    /// JThreadTeam is a finite-state machine, and these are the states.
    enum class Status {
        Idle, Running, Stopping
    };

    enum class Response {
        Success, InProgress, NotRunning, AlreadyRunning
    };


private:

    Status _status;
    std::vector<JWorker*> _workers;
    JScheduler& _scheduler;


public:

    JThreadTeam(JScheduler& scheduler);

    ~JThreadTeam();

    /// Returns the current state of the JThreadTeam
    Status get_status();

    /// Reports the current activity of each Worker
    std::vector<JWorker::Summary> get_worker_summaries();

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
    JLogger logger;
};



