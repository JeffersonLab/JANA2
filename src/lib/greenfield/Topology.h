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
        std::string queue_name;
        uint64_t message_count;
        uint64_t threshold;
        bool is_active;
    };

    /// POD type used for inspecting arrows.
    /// This helps separate the external API from the internal implementation.
    struct ArrowStatus {
        std::string arrow_name;
        bool is_active;
        bool is_parallel;
        uint32_t thread_count;
        uint64_t messages_completed;
        uint32_t chunksize;
        double avg_latency;
        double inst_latency;
        double avg_overhead;
        double inst_overhead;
        double throughput;

        ArrowStatus(Arrow* arrow);
    };


    // TODO: Make these private
    std::map<std::string, Arrow *> arrows;
    std::vector<QueueBase *> queues;

private:
    std::vector<Component *> components;


public:
    /// Leave the logger accessible for now so we can potentially inject it during testing
    std::shared_ptr<JLogger> logger;

    Topology() = default;
    ~Topology();

    void addManagedComponent(Component *component);
    void addArrow(Arrow *arrow);
    void addQueue(QueueBase *queue);
    void finalize();  // TODO: get rid of finalize so that we can dynamically add arrows

    void activate(std::string arrow_name);
    void deactivate(std::string arrow_name);

    std::vector<ArrowStatus> get_arrow_status();
    std::vector<QueueStatus> get_queue_status();
    ArrowStatus get_status(const std::string &arrow_name);
    void log_status();

    // get_graph()              // perhaps build an interactive visual someday

    // TODO: These don't belong here. Maybe in a TopologyDebugger instead?
    bool is_active();
    StreamStatus step(const std::string &arrow_name);
    // run_arrow(string arrow_name)
    // run_message(string source_name)
    // run_sequentially()
    // run()

};

} // namespace greenfield

#endif // GREENFIELD_TOPOLOGY_H


