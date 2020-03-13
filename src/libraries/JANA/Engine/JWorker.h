//
// Created by nbrei on 4/4/19.
//

#ifndef JANA_JWORKER_H
#define JANA_JWORKER_H

#include <thread>
#include <JANA/Services/JLoggingService.h>
#include <JANA/Engine/JScheduler.h>
#include <JANA/Engine/JWorkerMetrics.h>
#include <JANA/Engine/JArrowPerfSummary.h>


using jclock_t = std::chrono::steady_clock;
using duration_t = std::chrono::steady_clock::duration;

class JWorker {
    /// Designed so that the Worker checks in with the Scheduler on his own terms;
    /// i.e. nobody will update the worker's assignment externally. This eliminates
    /// a whole lot of synchronization since we can assume
    /// that the Worker's internal state won't be updated by another thread.

public:
    /// The Worker may be configured to try different backoff strategies
    enum class RunState { Running, Stopping, Stopped };

    /// The logger is made public so that somebody else may set it
    JLogger logger;

private:
    /// Machinery that nobody else should modify. These should be protected eventually.
    /// Probably simply make them private and expose via get_status() -> Worker::Status
    JScheduler* _scheduler;
    unsigned _worker_id;
    unsigned _cpu_id;
    unsigned _location_id;
    bool _pin_to_cpu;
    std::atomic<RunState> _run_state;
    JArrow* _assignment;
    std::thread* _thread;    // JWorker encapsulates a thread of some kind. Nothing else should care how.
    JWorkerMetrics _worker_metrics;
    JArrowMetrics _arrow_metrics;
    std::mutex _assignment_mutex;

public:
    JWorker(JScheduler* scheduler, unsigned worker_id, unsigned cpu_id, unsigned domain_id, bool pin_to_cpu);
    ~JWorker();

    /// If we copy or move the Worker, the underlying std::thread will be left with a
    /// dangling pointer back to `this`. So we forbid copying, assigning, and moving.

    JWorker(const JWorker &other) = delete;
    JWorker(JWorker &&other) = delete;
    JWorker &operator=(const JWorker &other) = delete;

    RunState get_runstate() { return _run_state; };

    void start();
    void request_stop();
    void wait_for_stop();

    /// This is what the encapsulated thread is supposed to be doing
    void loop();

    /// Summarize what/how this Worker is doing. This is meant to be called from
    /// JProcessingController::measure_perf()
    void measure_perf(WorkerSummary& result);

};


#endif
