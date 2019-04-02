#ifndef GREENFIELD_TOPOLOGY_H
#define GREENFIELD_TOPOLOGY_H

#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>
#include <greenfield/Components.h>
#include <greenfield/Logger.h>

namespace greenfield {

class ThreadManager;
class Scheduler;

class Topology {
friend class RoundRobinScheduler;

public:

    /// POD type used for inspecting topology performance.
    /// This helps separate the external API from the internal implementation.
    struct TopologyStatus {
        uint64_t messages_completed;
        double uptime;
        double avg_throughput;
        double inst_throughput;
        double seq_bottleneck;
        double efficiency;
        uint64_t scheduler_visits;
        double scheduler_overhead;
    };

    /// POD type used for inspecting queue performance.
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
        double queue_overhead;
        uint64_t queue_visits;

        ArrowStatus(Arrow* arrow);
    };


    // TODO: Make these private
    std::vector<Arrow *> arrows;
    std::vector<QueueBase *> queues;

private:
    std::map<std::string, Arrow *> arrow_lookup;
    std::vector<Component *> components;
    std::vector<Arrow*> sinks;  // Used to get total finished messages

    bool _is_running = false;
    Scheduler* _scheduler = nullptr;
    ThreadManager* _threadManager = nullptr;


    uint32_t _ncpus;
    std::chrono::time_point<std::chrono::steady_clock> _start_time;
    std::chrono::time_point<std::chrono::steady_clock> _last_time;
    std::chrono::steady_clock::duration _scheduler_time;
    uint64_t _last_message_count = 0;
    uint64_t _scheduler_visits = 0;  // TODO: These belong on Scheduler instead?


public:
    /// Leave the logger accessible for now so we can potentially inject it during testing
    Logger logger;

    Topology() = default;
    virtual ~Topology();

    void addManagedComponent(Component *component);
    void addArrow(Arrow *arrow, bool sink=false);
    void addQueue(QueueBase *queue);
    void finalize();  // TODO: get rid of finalize so that we can dynamically add arrows
    Arrow* get_arrow(std::string arrow_name);  // TODO: Should be used internally, not as part of API

    void activate(std::string arrow_name);
    void deactivate(std::string arrow_name);

    TopologyStatus get_topology_status();
    std::vector<ArrowStatus> get_arrow_status();
    std::vector<QueueStatus> get_queue_status();
    ArrowStatus get_status(const std::string &arrow_name);
    void log_status();

    // get_graph()              // perhaps build an interactive visual someday

    // TODO: These don't belong here. Maybe in a TopologyDebugger instead?
    bool is_active();
    StreamStatus step(const std::string &arrow_name);
    void run(int threads);
    // run_arrow(string arrow_name)
    // run_message(string source_name)
    // run_sequentially()
    // run()

};

} // namespace greenfield

#endif // GREENFIELD_TOPOLOGY_H


