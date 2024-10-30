
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <thread>
#include <JANA/Engine/JScheduler.h>
#include <JANA/Engine/JWorkerMetrics.h>
#include <JANA/Engine/JPerfSummary.h>
#include <atomic>


class JArrowProcessingController;

class JWorker {
    /// Designed so that the Worker checks in with the Scheduler on his own terms;
    /// i.e. nobody will update the worker's assignment externally. This eliminates
    /// a whole lot of synchronization since we can assume
    /// that the Worker's internal state won't be updated by another thread.

public:
    enum class RunState { Running, Stopping, Stopped, TimedOut, Excepted };

    /// The logger is made public so that somebody else may set it
    JLogger logger;

private:
    /// Machinery that nobody else should modify. These should be protected eventually.
    /// Probably simply make them private and expose via get_status() -> Worker::Status
    JScheduler* m_scheduler;
    JArrowProcessingController* m_japc; // This is used to turn off the other workers if an exception happens
    unsigned m_worker_id;
    unsigned m_cpu_id;
    unsigned m_location_id;
    bool m_pin_to_cpu;
    std::atomic<RunState> m_run_state;
    JArrow* m_assignment;
    std::thread* m_thread;    // JWorker encapsulates a thread of some kind. Nothing else should care how.
    JWorkerMetrics m_worker_metrics;
    JArrowMetrics m_arrow_metrics;
    std::mutex m_assignment_mutex;
    JException m_exception;

public:
    JWorker(JArrowProcessingController* japc, JScheduler* scheduler, unsigned worker_id, unsigned cpu_id, unsigned domain_id, bool pin_to_cpu);
    ~JWorker();

    /// If we copy or move the Worker, the underlying std::thread will be left with a
    /// dangling pointer back to `this`. So we forbid copying, assigning, and moving.

    JWorker(const JWorker &other) = delete;
    JWorker(JWorker &&other) = delete;
    JWorker &operator=(const JWorker &other) = delete;

    RunState get_runstate() { return m_run_state; };

    void start();
    void request_stop();
    void wait_for_stop();
    void declare_timeout();
    const JException& get_exception() const;

    /// This is what the encapsulated thread is supposed to be doing
    void loop();

    /// Summarize what/how this Worker is doing. This is meant to be called from
    /// JProcessingController::measure_perf()
    void measure_perf(WorkerSummary& result);

};

