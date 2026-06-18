
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Engine/JExecutionEngine.h>
#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JTopologyBuilder.h>
#include <JANA/Utils/JBacktrace.h>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <exception>


class JExecutionEngine_Static : public JExecutionEngine {

public:
    using clock_t = std::chrono::steady_clock;

    enum class InterruptStatus { NoInterruptsSupervised, NoInterruptsUnsupervised, InspectRequested, InspectInProgress, PauseAndQuit };

    struct Worker {
        size_t worker_id;
        JBacktrace* backtrace = nullptr;
        bool block_until_next_task = true;
        bool stop_requested = false;

        bool has_active_task = false;
        JArrow* arrow = nullptr;
        JEvent* event = nullptr;

        JArrow::FireResult output_status = JArrow::FireResult::NotRunYet;
        size_t output_count = 0;
        JArrow::OutputData outputs;

        inline void Fire() {
            arrow->Fire(event, outputs, output_count, output_status);
        }
    };

#ifndef JANA2_TESTCASE
private:
#endif

    struct ArrowControlBlock {
        enum class Status { Paused, Running, Finished };

        size_t arrow_id = 0;
        Status status = Status::Paused;
        bool is_parallel = false;
        bool is_source = false;
        bool is_sink = false;
        size_t next_input = 0;
        size_t active_tasks = 0;
        size_t total_workers = 0;
        size_t events_processed;
        clock_t::duration total_processing_duration;
    };

    struct WorkerControlBlock {
        size_t worker_id = 0;
        size_t arrow_id = 0;
        size_t worker_nr = 0;
        size_t cpu_id = 0;
        size_t location_id = 0;
        bool is_event_warmed_up = false;
        bool is_stop_requested = false;
        bool is_timed_out = false;
        bool has_task = false;
        clock_t::time_point last_checkout_time = clock_t::now();
        uint64_t last_event_nr = 0;
        JBacktrace backtrace;
        std::exception_ptr stored_exception = nullptr;
        std::thread* thread = nullptr;
    };


#ifndef JANA2_TESTCASE
private:
#endif
    // Services
    Service<JTopologyBuilder> m_topology {this};

    // Parameters
    size_t m_requested_parallelism = 0;
    size_t m_worker_parallelism = 0;
    size_t m_queue_parallelism = 0;
    std::map<JEventLevel, size_t> m_max_inflight_events;
    bool m_show_ticker = true;
    bool m_enable_timeout = true;
    int m_backoff_ms = 10;
    int m_ticker_ms = 500;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;
    std::string m_path_to_named_pipe = "/tmp/jana_status";

    // Concurrency
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::vector<std::unique_ptr<WorkerControlBlock>> m_worker_states;
    std::vector<ArrowControlBlock> m_arrow_states;
    RunStatus m_runstatus = RunStatus::Paused;
    std::atomic<InterruptStatus> m_interrupt_status { InterruptStatus::NoInterruptsUnsupervised };
    std::atomic_bool m_print_worker_report_requested {false};
    std::atomic_bool m_send_worker_report_requested {false};
    size_t m_next_arrow_id=0;
    size_t m_arrow_count=0;

    // Metrics
    size_t m_event_count_at_start = 0;
    size_t m_event_count_at_finish = 0;
    clock_t::time_point m_time_at_start;
    clock_t::time_point m_time_at_finish;
    clock_t::duration m_total_idle_duration = clock_t::duration::zero();
    clock_t::duration m_total_scheduler_duration = clock_t::duration::zero();


public:

    JExecutionEngine_Static() {
        SetLoggerName("jana");
    }

    ~JExecutionEngine_Static() {
        ScaleWorkers(0);
        // If we don't shut down the thread team, the condition variable will hang during destruction
    }

    void Init() override;
    void RunSupervisor() override;

    void RunTopology() override;
    void PauseTopology() override;
    void DrainTopology() override;
    void FinishTopology() override;

    void ScaleWorkers() override;
    void ScaleWorkers(size_t requested_parallelism) override;
    Worker RegisterWorker(std::string arrow_name);
    void RunWorker(Worker);

    JArrow::FireResult Fire(size_t arrow_id, size_t location_id=0) override;

    Perf GetPerf() override;
    RunStatus GetRunStatus() override;
    void SetTickerEnabled(bool ticker_on) override;
    bool IsTickerEnabled() const override;
    void SetTimeoutEnabled(bool timeout_on) override;
    bool IsTimeoutEnabled() const override;
    void RequestInspector() override;

    void HandleSIGINT() override;
    void HandleSIGUSR1() override;
    void HandleSIGUSR2() override;
    void HandleSIGTSTP() override;

#ifndef JANA2_TESTCASE
private:
#endif

    void PrintWorkerReport(bool);
    void PrintFinalReport();
    bool CheckTimeout();
    void HandleFailures();

    void ScaleQueues();
    void ExchangeTask(Worker& worker);
    void ReturnResult_Unsafe(Worker& worker, clock_t::time_point checkin_time);
    bool FindNextReadyTask_Unsafe(Worker& worker);
    void DetectPause_Unsafe();
    size_t CalculateMaxInflightEvents(JEventLevel level);

};


std::string ToString(JExecutionEngine::RunStatus status);


