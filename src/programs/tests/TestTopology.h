#ifndef GREENFIELD_TOPOLOGY_H
#define GREENFIELD_TOPOLOGY_H

#include <JANA/JArrow.h>
#include <JANA/JLogger.h>

class JScheduler;
class JWorker;

using jclock_t = std::chrono::steady_clock;

class TestTopology : public JActivable {

public:


    /// POD type used for inspecting topology performance.
    /// This helps separate the external API from the internal implementation.
    struct TopologyStatus {
        size_t messages_completed;
        double uptime_s;
        double avg_throughput_hz;
        double inst_throughput_hz;
        double seq_bottleneck_hz;
        double par_bottleneck_hz;
        double efficiency_frac;
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

        ArrowStatus(JArrow* arrow);
    };


    // TODO: Make these private
    std::vector<JArrow *> arrows;
    std::vector<JWorker *> workers;

private:

    enum class RunState { BeforeRun, DuringRun, AfterRun };

    std::map<std::string, JArrow *> arrow_lookup;
    std::vector<JArrow*> sinks;            // So we can get total finished messages

    JScheduler* _scheduler = nullptr;


    RunState _run_state = RunState::BeforeRun;
    jclock_t::time_point _start_time;
    jclock_t::time_point _last_time;
    jclock_t::time_point _stop_time;
    size_t _last_message_count = 0;
    uint32_t _ncpus;

    TopologyStatus get_topology_status(std::map<JArrow*, ArrowStatus>& statuses);

public:
    /// Leave the logger accessible for now so we can potentially inject it during testing
    JLogger logger;

    TestTopology() {
        logger = JLoggingService::logger("JTopology");
    }

    virtual ~TestTopology();

    void addArrow(JArrow *arrow, bool sink=false);

    JArrow* get_arrow(std::string arrow_name);  // TODO: Should be used internally, not as part of API

    void activate(std::string arrow_name);
    void deactivate(std::string arrow_name);

    TopologyStatus get_topology_status();
    std::vector<ArrowStatus> get_arrow_status();
    std::vector<QueueStatus> get_queue_status();
    ArrowStatus get_status(const std::string &arrow_name);
    void log_status();

    // get_graph()              // perhaps build an interactive visual someday

    bool is_active() override;
    void set_active(bool is_active) override;

    JArrowMetrics::Status step(const std::string &arrow_name);
    void run(int threads);
    void wait_until_finished();
};


#endif // GREENFIELD_TOPOLOGY_H


