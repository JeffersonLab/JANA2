#ifndef GREENFIELD_TOPOLOGY_H
#define GREENFIELD_TOPOLOGY_H

#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>
#include <greenfield/Components.h>
#include <greenfield/Logger.h>

namespace greenfield {

class ThreadManager;
class Scheduler;

using clock_t = std::chrono::steady_clock;

class Topology {
friend class RoundRobinScheduler;

public:


    /// POD type used for inspecting topology performance.
    /// This helps separate the external API from the internal implementation.
    struct TopologyStatus {
        size_t messages_completed;
        size_t scheduler_visit_count;
        double uptime_s;
        double avg_throughput_hz;
        double inst_throughput_hz;
        double seq_bottleneck_hz;
        double par_bottleneck_hz;
        double efficiency_frac;
        double scheduler_overhead_frac;
    };

    /// POD type used for inspecting queue performance.
    /// This helps separate the external API from the internal implementation.
    struct QueueStatus {
        std::string queue_name;
        size_t message_count;
        size_t threshold;
        bool is_active;
    };

    /// POD type used for inspecting arrows.
    /// This helps separate the external API from the internal implementation.
    struct ArrowStatus {
        std::string arrow_name;
        bool is_active;
        bool is_parallel;
        size_t thread_count;
        size_t messages_completed;
        size_t chunksize;
        double avg_latency_ms;
        double inst_latency_ms;
        double queue_overhead_frac;
        size_t queue_visit_count;

        ArrowStatus(Arrow* arrow);
    };


    // TODO: Make these private
    std::vector<Arrow *> arrows;
    std::vector<QueueBase *> queues;

private:

    enum class RunState { BeforeRun, DuringRun, AfterRun };

    std::map<std::string, Arrow *> arrow_lookup;
    std::vector<Component *> components;  // So we can delete Components which we own
    std::vector<Arrow*> sinks;            // So we can get total finished messages

    Scheduler* _scheduler = nullptr;
    ThreadManager* _threadManager = nullptr;


    RunState _run_state = RunState::BeforeRun;
    clock_t::time_point _start_time;
    clock_t::time_point _last_time;
    clock_t::time_point _stop_time;
    clock_t::duration _scheduler_time = clock_t::duration::zero();
    size_t _last_message_count = 0;
    size_t _scheduler_visits = 0;  // TODO: These belong on Scheduler instead?
    uint32_t _ncpus;

    TopologyStatus get_topology_status(std::map<Arrow*, ArrowStatus>& statuses);

public:
    /// Leave the logger accessible for now so we can potentially inject it during testing
    Logger logger;

    Topology() = default;
    virtual ~Topology();

    void addManagedComponent(Component *component);
    void addArrow(Arrow *arrow, bool sink=false);
    void addQueue(QueueBase *queue);

    Arrow* get_arrow(std::string arrow_name);  // TODO: Should be used internally, not as part of API

    void activate(std::string arrow_name);
    void deactivate(std::string arrow_name);

    TopologyStatus get_topology_status();
    std::vector<ArrowStatus> get_arrow_status();
    std::vector<QueueStatus> get_queue_status();
    ArrowStatus get_status(const std::string &arrow_name);
    void log_status();

    // get_graph()              // perhaps build an interactive visual someday

    bool is_active();
    StreamStatus step(const std::string &arrow_name);
    void run(int threads);
    void wait_until_finished();
    // run_message(string source_name)
    // run_sequentially()

};

} // namespace greenfield

#endif // GREENFIELD_TOPOLOGY_H


