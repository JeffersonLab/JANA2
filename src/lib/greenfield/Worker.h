//
// Created by nbrei on 4/4/19.
//

#ifndef GREENFIELD_WORKER_H
#define GREENFIELD_WORKER_H

#include <thread>
#include <greenfield/Logger.h>
#include <greenfield/Scheduler.h>

namespace greenfield {

using clock_t = std::chrono::steady_clock;
using duration_t = std::chrono::steady_clock::duration;

class Worker {
    /// Designed so that the Worker checks in with the Scheduler on his own terms;
    /// i.e. nobody will update the worker's assignment externally. This eliminates
    /// a whole lot of synchronization since we can assume
    /// that the Worker's internal state won't be updated by another thread.

private:
    std::thread *_thread;    // Worker encapsulates a thread of some kind. Nothing else should care how.
    Scheduler &_scheduler;

public:

    /// Worker also need to maintain metrics. Similar to Arrow::Metrics, these form a monoid
    /// where the identity element is (0,0,0) and the combine operation accumulates totals
    /// in a thread-safe way. Alas, the combine operation mutates state for performance reasons.
    /// We've separated Metrics from the Worker itself because it is not always obvious
    /// who should be performing the accumulation or when, and this gives us the freedom to
    /// try different possibilities.

    class Metrics {

        duration_t _useful_time;
        duration_t _retry_time;
        duration_t _scheduler_time;
        duration_t _idle_time;
        std::mutex _mutex;

    public:

        Metrics();

        void update(const Metrics &other);

        void update(const duration_t& useful_time,
                    const duration_t& retry_time,
                    const duration_t& scheduler_time,
                    const duration_t& idle_time);

        void get(duration_t& useful_time,
                 duration_t& retry_time,
                 duration_t& scheduler_time,
                 duration_t& idle_time);
    };

    /// Exposes a view of this Worker's performance to the outside world.
    /// This is deliberately a POD type.
    struct Summary {

        int worker_id;
        std::string last_arrow_name;
        double useful_time_frac;
        double retry_time_frac;
        double idle_time_frac;
        double scheduler_time_frac;
    };


    /// The Worker may be configured to try different backoff strategies
    enum class BackoffStrategy {
        Constant, Linear, Exponential
    };


    /// Machinery that nobody else should modify. These should be protected eventually.
    /// Probably simply make them private and expose via get_status() -> Worker::Status
    const uint32_t worker_id;
    Arrow *assignment = nullptr;
    bool shutdown_requested = false;   // For communicating with ThreadManager
    bool shutdown_achieved = false;    // TODO: Make these atomic
    Logger _logger;
    Metrics metrics;


    /// Configuration options
    int backoff_tries = 4;
    BackoffStrategy backoff_strategy = BackoffStrategy::Exponential;
    duration_t initial_backoff_time = std::chrono::microseconds(1000);
    duration_t checkin_time = std::chrono::milliseconds(500);


    /// Worker serves as an RAII wrapper around a thread of some kind,
    /// which runs for the lifetime of the Worker.

    Worker(uint32_t id, Scheduler &scheduler);
    ~Worker();

    /// If we copy or move the Worker, the underlying std::thread will be left with a
    /// dangling pointer back to `this`. So we forbid copying, assigning, and moving.

    Worker(const Worker &other) = delete;
    Worker(Worker &&other) = delete;
    Worker &operator=(const Worker &other) = delete;


    /// This is what the encapsulated thread is supposed to be doing
    void loop();

    /// Summarize what/how this Worker is doing
    Summary get_summary();
};

} // namespace greenfield

#endif
