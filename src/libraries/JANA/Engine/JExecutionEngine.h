
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JService.h>
#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Utils/JBacktrace.h>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <exception>

extern thread_local int jana2_worker_id;

class JExecutionEngine : public JService {

public:
    using clock_t = std::chrono::steady_clock;

    enum class RunStatus { Paused, Running, Pausing, Draining, Failed, Finished };
    enum class InterruptStatus { NoInterruptsSupervised, NoInterruptsUnsupervised, InspectRequested, InspectInProgress, PauseAndQuit };

    struct Perf {
        RunStatus runstatus;
        size_t thread_count;
        size_t event_count;
        size_t uptime_ms;
        double throughput_hz;
        JEventLevel event_level;
    };

#ifndef JANA2_TESTCASE
private:
#endif

    struct Task {
        JArrow* arrow = nullptr;
        JEvent* input_event = nullptr;
        int input_port = -1;
        JArrow::OutputData outputs;
        size_t output_count = 0;
        JArrow::FireResult status = JArrow::FireResult::NotRunYet;
    };

    struct ArrowState {
        enum class Status { Paused, Running, Finished };
        Status status = Status::Paused;
        bool is_parallel = false;
        bool is_source = false;
        bool is_sink = false;
        size_t next_input = 0;
        size_t active_tasks = 0;
        size_t events_processed;
        clock_t::duration total_processing_duration;
    };

    struct WorkerState {
        std::thread* thread = nullptr;
        size_t worker_id = 0;
        size_t cpu_id = 0;
        size_t location_id = 0;
        clock_t::time_point last_checkout_time = clock_t::now();
        std::exception_ptr stored_exception = nullptr;
        bool is_stop_requested = false;
        bool is_event_warmed_up = false;
        bool is_timed_out = false;
        uint64_t last_event_nr = 0;
        size_t last_arrow_id = 0;
        JBacktrace backtrace;
    };


#ifndef JANA2_TESTCASE
private:
#endif
    // Services
    Service<JTopologyBuilder> m_topology {this};

    // Parameters
    bool m_show_ticker = true;
    bool m_enable_timeout = true;
    int m_backoff_ms = 10;
    int m_ticker_ms = 500;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;

    // Concurrency
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::vector<std::unique_ptr<WorkerState>> m_worker_states;
    std::vector<ArrowState> m_arrow_states;
    RunStatus m_runstatus = RunStatus::Paused;
    std::atomic<InterruptStatus> m_interrupt_status { InterruptStatus::NoInterruptsUnsupervised };

    // Metrics
    size_t m_event_count_at_start = 0;
    size_t m_event_count_at_finish = 0;
    clock_t::time_point m_time_at_start;
    clock_t::time_point m_time_at_finish;
    clock_t::duration m_total_idle_duration = clock_t::duration::zero();
    clock_t::duration m_total_scheduler_duration = clock_t::duration::zero();


public:

    JExecutionEngine() {
        SetLoggerName("jana");
    }

    ~JExecutionEngine() {
        Scale(0);
        // If we don't shut down the thread team, the condition variable will hang during destruction
    }

    void Init() override;

    void Run();
    void Scale(size_t nthreads);
    void RequestPause();
    void RequestDrain();
    void Wait();
    void Finish();

    JArrow::FireResult Fire(size_t arrow_id, size_t location_id=0);

    Perf GetPerf();
    RunStatus GetRunStatus();
    void SetTickerEnabled(bool ticker_on);
    bool IsTickerEnabled() const;
    void SetTimeoutEnabled(bool timeout_on);
    bool IsTimeoutEnabled() const;

    void HandleSIGINT();
    void HandleSIGUSR1();
    void HandleSIGUSR2();

#ifndef JANA2_TESTCASE
private:
#endif

    std::pair<int, JBacktrace*> RegisterExternalWorker();
    void PrintFinalReport();
    bool CheckTimeout();
    void HandleFailures();
    void RunWorker(size_t worker_id, JBacktrace* worker_backtrace);
    void ExchangeTask(Task& task, size_t worker_id, bool nonblocking=false);
    void CheckinCompletedTask_Unsafe(Task& task, WorkerState& worker, clock_t::time_point checkin_time);
    void FindNextReadyTask_Unsafe(Task& task, WorkerState& worker);

};




