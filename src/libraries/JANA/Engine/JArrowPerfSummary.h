
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JARROWPERFSUMMARY_H
#define JANA2_JARROWPERFSUMMARY_H


#include <JANA/Status/JPerfSummary.h>
#include <JANA/Engine/JArrow.h>

#include <vector>
#include <string>

struct ArrowSummary {
    std::string arrow_name;
    bool is_parallel;
    size_t thread_count;
    JArrow::NodeType arrow_type;
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

struct JArrowPerfSummary : public JPerfSummary {

    double avg_seq_bottleneck_hz;
    double avg_par_bottleneck_hz;
    double avg_efficiency_frac;

    std::vector<WorkerSummary> workers;
    std::vector<ArrowSummary> arrows;

    JArrowPerfSummary() = default;
    JArrowPerfSummary(const JArrowPerfSummary&) = default;
    virtual ~JArrowPerfSummary() = default;

};

std::ostream& operator<<(std::ostream& stream, const JArrowPerfSummary& data);


#endif //JANA2_JARROWPERFSUMMARY_H
