
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once
#include <vector>
#include <string>

struct ArrowSummary {
    std::string arrow_name;
    bool is_parallel;
    bool is_source;
    bool is_sink;
    size_t thread_count;
    int running_upstreams;
    bool has_backpressure;
    size_t messages_pending;
    size_t threshold;
    size_t chunksize;

    size_t total_messages_completed;
    size_t last_messages_completed;
    double avg_latency_ms;
    double avg_queue_latency_ms;
    double last_latency_ms;
    double last_queue_latency_ms;
    double avg_queue_overhead_frac;
    size_t queue_visit_count;
};

struct WorkerSummary {
    int worker_id;
    int cpu_id;
    bool is_pinned;
    double last_heartbeat_ms;
    double total_useful_time_ms;
    double total_retry_time_ms;
    double total_idle_time_ms;
    double total_scheduler_time_ms;
    double last_useful_time_ms;
    double last_retry_time_ms;
    double last_idle_time_ms;
    double last_scheduler_time_ms;
    long scheduler_visit_count;
    std::string last_arrow_name;
    double last_arrow_avg_latency_ms;
    double last_arrow_avg_queue_latency_ms;
    double last_arrow_last_latency_ms;
    double last_arrow_last_queue_latency_ms;
    size_t last_arrow_queue_visit_count;
};

struct JPerfSummary {

    size_t monotonic_events_completed = 0;  // Since program started
    size_t total_events_completed = 0;      // Since run or rescale started
    size_t latest_events_completed = 0;     // Since previous measurement
    size_t thread_count = 0;
    double total_uptime_s = 0;
    double latest_uptime_s = 0;
    double avg_throughput_hz = 0;
    double latest_throughput_hz = 0;

    double avg_seq_bottleneck_hz;
    double avg_par_bottleneck_hz;
    double avg_efficiency_frac;

    std::vector<WorkerSummary> workers;
    std::vector<ArrowSummary> arrows;

};

std::ostream& operator<<(std::ostream& stream, const JPerfSummary& data);


