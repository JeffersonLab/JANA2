#ifndef GREENFIELD_TOPOLOGY_H
#define GREENFIELD_TOPOLOGY_H

#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>
#include <greenfield/Components.h>
#include <greenfield/JLogger.h>

namespace greenfield {

class Topology {
friend class RoundRobinScheduler;

public:
    /// POD type used for inspecting queues.
    /// This helps separate the external API from the internal implementation.
    struct QueueStatus {
        uint32_t queue_id;
        uint64_t message_count;
        uint64_t message_count_threshold;
        bool is_finished;
    };

    /// POD type used for inspecting arrows.
    /// This helps separate the external API from the internal implementation.
    struct ArrowStatus {
        uint32_t arrow_id;
        std::string arrow_name;
        bool is_finished;
        bool is_parallel;
        uint32_t thread_count;
        uint64_t messages_completed;
        double short_term_avg_latency;
        double long_term_avg_latency;
    };


    // TODO: Make these private
    std::map<std::string, Arrow *> arrows;
    std::vector<QueueBase *> queues;

private:
    std::vector<Component *> components;

    std::vector<ArrowStatus> _arrow_statuses;
    std::vector<bool> finished_queues;
    std::vector<bool> finished_matrix;
    int arrow_count;
    int queue_count;


public:
    /// Leave the logger accessible for now so we can potentially inject it during testing
    std::shared_ptr<JLogger> logger;

    Topology() = default;
    ~Topology();

    int next_index();  // TODO: get rid of this
    void addManagedComponent(Component *component);
    void addArrow(Arrow *arrow);
    void addQueue(QueueBase *queue);
    void finalize();  // TODO: get rid of finalize so that we can dynamically add arrows

    void activate(std::string arrow_name);
    void deactivate(std::string arrow_name);

    void update(Arrow *arrow, StreamStatus last_result, double latency, uint64_t messages_completed);

    std::vector<ArrowStatus> get_arrow_status();
    std::vector<QueueStatus> get_queue_status();

    void log_arrow_status();
    void log_queue_status();

    // get_graph()              // perhaps build an interactive visual someday

    // TODO: These don't belong here. Maybe in a TopologyDebugger instead?
    StreamStatus step(const std::string &arrow_name);
    // run_arrow(string arrow_name)
    // run_message(string source_name)
    // run_sequentially()
    // run()

private:
    void report_arrow_finished(Arrow *arrow);

};

} // namespace greenfield

#endif // GREENFIELD_TOPOLOGY_H


