
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JService.h>
#include <JANA/Topology/JTopologyBuilder.h>

#include <map>
#include <chrono>
#include <condition_variable>


class JExecutionEngine : public JService {

public:
    using clock_t = std::chrono::high_resolution_clock;
    using duration_t = std::chrono::duration<float, std::milli>;

    enum class RunStatus { Paused, Running, Pausing, Draining, Failed, Finished };

    struct Perf {
        double throughput_hz;
        duration_t uptime;
        size_t event_count;
        JEventLevel event_level;
    };

    struct Worker {
        std::thread* thread;
        size_t worker_id;
        size_t cpu_id;
        size_t location_id;
        clock_t::time_point last_checkin_time;
        JException stored_exception;
    };

    struct FakeArrow {
        std::string name;
        bool is_parallel;
        bool is_source;
        bool is_finished;
        size_t next_input = 0;
    };

    struct Task {
        FakeArrow* arrow;
        size_t input_index;
        size_t event_nr;
    };

    struct SchedulerState {
        bool is_parallel = false;
        bool is_source = false;
        bool is_finished = false;
        size_t next_input = 0;
        size_t active_tasks = 0;
    };


private:
    
    // Services
    Service<JTopologyBuilder> m_topology {this};

    // Parameters
    size_t m_nthreads = 1;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;
    bool m_pin_worker_to_cpu = true;

    // Concurrency
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::vector<Worker> m_workers;
    std::map<std::string, SchedulerState> m_scheduler_state;
    RunStatus m_runstatus;

    // Metrics
    size_t m_event_count_at_start;
    size_t m_event_count_at_finish;
    clock_t::time_point m_time_at_start;
    clock_t::time_point m_time_at_finish;


public:

    void Init() override;

    void Run();
    void Scale(size_t nthreads);
    void RequestPause();
    void RequestDrain();
    void Wait();
    void Finish();

    RunStatus GetRunStatus();
    Perf GetPerf();

    void RunSupervisor();
    void RunWorker(Worker& worker);

    std::optional<Task> ExchangeTask(std::optional<Task> completed_task);
    void FinishTaskUnsafe(Task finished_task);
    std::optional<Task> FindNextReadyTaskUnsafe();

};




