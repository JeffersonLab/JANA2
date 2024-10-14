
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JService.h>
#include <JANA/Topology/JTopologyBuilder.h>

#include <condition_variable>


class JExecutionEngine : public JService {

public:
    enum class Status { Paused, Running, Pausing, Draining, Failed };

    struct Perf {
        double throughput_hz;
        size_t uptime_ms;
        size_t event_count;
        JEventLevel event_level;
    };

    struct Worker {
        std::thread* thread;
        size_t worker_id;
        size_t cpu_id;
        size_t location_id;
        size_t last_checkin_time;
        JException stored_exception;
    };

    struct Task {
        size_t data;
        // Arrow
        // Input queue index
        // Event* eventually
    };


private:
    Service<JTopologyBuilder> m_topology;

    // Parameters
    size_t m_nthreads = 1;
    int m_timeout_s = 8;
    int m_warmup_timeout_s = 30;
    bool m_pin_worker_to_cpu = true;

    // Status and perf
    std::mutex m_mutex;
    std::condition_variable m_condvar;
    std::vector<Worker> m_workers;
    size_t m_event_count_at_start;
    size_t m_event_count_at_finish;
    size_t m_time_at_start;
    size_t m_time_at_finish;


public:
    JExecutionEngine();
    ~JExecutionEngine();

    void Init() override;

    void Run();
    void Scale(size_t nthreads);
    void RequestPause();
    void RequestDrain();
    void Wait();
    void Finish();

    Status GetStatus();
    Perf GetPerf();

    void RunSupervisor();
    void RunWorker();

    void AddTask(Task a);
    Task ExchangeTask(Task completed_task);

};




