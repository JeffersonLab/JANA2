
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Topology/JArrowMetrics.h"
#include <JANA/JService.h>
#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JTopologyBuilder.h>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <exception>
#include <memory>


class JExecutionEngine : public JService {

public:
    using clock_t = std::chrono::steady_clock;

    enum class RunStatus { Paused, Running, Pausing, Draining, Failed, Finished };

    struct Worker {
        std::thread* thread = nullptr;
        size_t worker_id = 0;
        size_t cpu_id = 0;
        size_t location_id = 0;
        clock_t::time_point last_checkout_time = clock_t::now();
        std::exception_ptr stored_exception = nullptr;
        bool is_stop_requested = false;
        bool is_timed_out = false;
    };

    struct Perf {
        RunStatus runstatus;
        size_t thread_count;
        size_t event_count;
        size_t uptime_ms;
        double throughput_hz;
        JEventLevel event_level;
    };

    struct Task {
        int arrow_id = -1;
        int worker_id = -1;
        JArrow* arrow = nullptr;
        JEvent* input_event = nullptr;
        int input_port = -1;
        JArrow::OutputData outputs;
        size_t output_count = 0;
        JArrowMetrics::Status status = JArrowMetrics::Status::NotRunYet;
    };

    struct SchedulerState {
        bool is_parallel = false;
        bool is_source = false;
        bool is_sink = false;
        bool is_active = true;
        size_t next_input = 0;
        size_t active_tasks = 0;
        size_t events_processed;
        clock_t::duration total_processing_duration;
    };


private:
    // Services
    Service<JTopologyBuilder> m_topology {this};

    // Parameters
    bool m_show_ticker = true;
    int m_backoff_ms = 10;
    int m_ticker_ms = 500;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;

    // Concurrency
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::vector<std::unique_ptr<Worker>> m_workers;
    std::vector<SchedulerState> m_scheduler_state;
    RunStatus m_runstatus = RunStatus::Paused;

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
    void Init() override;

    void Run();
    void Scale(size_t nthreads);
    void RequestPause();
    void RequestDrain();
    void Wait(bool finish=false);
    void Finish();

    RunStatus GetRunStatus();
    Perf GetPerf();

    int RegisterExternalWorker();
    void PrintFinalReport();
    void RunWorker(Worker& worker);
    void ExchangeTask(Task& task, bool nonblocking=false);
    void CheckinCompletedTask_Unsafe(Task& task, clock_t::time_point checkin_time);
    void FindNextReadyTask_Unsafe(Task& task);

};




