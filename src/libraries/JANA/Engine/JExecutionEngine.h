
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Topology/JArrowMetrics.h"
#include <JANA/JService.h>
#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JTopologyBuilder.h>

#include <chrono>
#include <condition_variable>
#include <map>


class JExecutionEngine : public JService {

public:
    using clock_t = std::chrono::high_resolution_clock;
    using duration_t = std::chrono::duration<float, std::milli>;

    enum class RunStatus { Paused, Running, Pausing, Draining, Failed, Finished };


    struct Worker {
        std::thread* thread;
        size_t worker_id;
        size_t cpu_id;
        size_t location_id;
        clock_t::time_point last_checkin_time;
        JException stored_exception;
    };

    struct Perf {
        size_t thread_count;
        size_t event_count;
        duration_t uptime;
        double throughput_hz;
        JEventLevel event_level;
    };

    struct Task {
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
        bool is_active = true;
        size_t next_input = 0;
        size_t active_tasks = 0;
        size_t events_processed;
        clock_t::duration total_useful_time;
    };


private:
    // Services
    Service<JTopologyBuilder> m_topology {this};

    // Parameters
    size_t m_nthreads = 1;
    int m_poll_ms = 10;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;
    bool m_pin_worker_to_cpu = true;

    // Concurrency
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::vector<Worker> m_workers;
    std::map<std::string, SchedulerState> m_scheduler_state;
    RunStatus m_runstatus = RunStatus::Paused;

    // Metrics
    size_t m_event_count_at_start = 0;
    size_t m_event_count_at_finish = 0;
    clock_t::time_point m_time_at_start;
    clock_t::time_point m_time_at_finish;


public:

    void Init() override;

    void Run();
    void Scale(size_t nthreads);
    void RequestPause();
    void RequestDrain();
    void Wait(bool finish=false);
    void Finish();

    RunStatus GetRunStatus();
    Perf GetPerf();

    void RunSupervisor();
    void RunWorker(Worker& worker);
    void ExchangeTask(Task& task);
    void IngestCompletedTask_Unsafe(Task& task);
    void FindNextReadyTask_Unsafe(Task& task, bool& found_task, bool& found_termination);

};




