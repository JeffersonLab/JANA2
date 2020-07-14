
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JWORKERMETRICS_H
#define JANA2_JWORKERMETRICS_H

#include <chrono>
#include <mutex>

class JWorkerMetrics {
    /// Workers need to maintain metrics. Similar to Arrow::Metrics, these form a monoid
    /// where the identity element is (0,0,0) and the combine operation accumulates totals
    /// in a thread-safe way. Alas, the combine operation mutates state for performance reasons.
    /// We've separated Metrics from the Worker itself because it is not always obvious
    /// who should be performing the accumulation or when, and this gives us the freedom to
    /// try different possibilities.


    using duration_t = std::chrono::steady_clock::duration;

    mutable std::mutex _mutex;
    // mutex is mutable so that we can lock before reading from a const ref

    long _scheduler_visit_count;

    duration_t _total_useful_time;
    duration_t _total_retry_time;
    duration_t _total_scheduler_time;
    duration_t _total_idle_time;
    duration_t _last_useful_time;
    duration_t _last_retry_time;
    duration_t _last_scheduler_time;
    duration_t _last_idle_time;

public:
    void clear() {
        _mutex.lock();
        _scheduler_visit_count = 0;

        auto zero = duration_t::zero();

        _total_useful_time = zero;
        _total_retry_time = zero;
        _total_scheduler_time = zero;
        _total_idle_time = zero;
        _last_useful_time = zero;
        _last_retry_time = zero;
        _last_scheduler_time = zero;
        _last_idle_time = zero;
        _mutex.unlock();
    }


    void update(const JWorkerMetrics &other) {

        _mutex.lock();
        other._mutex.lock();
        _scheduler_visit_count += other._scheduler_visit_count;
        _total_useful_time += other._total_useful_time;
        _total_retry_time += other._total_retry_time;
        _total_scheduler_time += other._total_scheduler_time;
        _total_idle_time += other._total_idle_time;
        _last_useful_time = other._last_useful_time;
        _last_retry_time = other._last_retry_time;
        _last_scheduler_time = other._last_scheduler_time;
        _last_idle_time = other._last_idle_time;
        other._mutex.unlock();
        _mutex.unlock();
    }


    void update(const long& scheduler_visit_count,
                const duration_t& useful_time,
                const duration_t& retry_time,
                const duration_t& scheduler_time,
                const duration_t& idle_time) {

        _mutex.lock();
        _scheduler_visit_count += scheduler_visit_count;
        _total_useful_time += useful_time;
        _total_retry_time += retry_time;
        _total_scheduler_time += scheduler_time;
        _total_idle_time += idle_time;
        _last_useful_time = useful_time;
        _last_retry_time = retry_time;
        _last_scheduler_time = scheduler_time;
        _last_idle_time = idle_time;
        _mutex.unlock();
    }


    void get(long& scheduler_visit_count,
             duration_t& total_useful_time,
             duration_t& total_retry_time,
             duration_t& total_scheduler_time,
             duration_t& total_idle_time,
             duration_t& last_useful_time,
             duration_t& last_retry_time,
             duration_t& last_scheduler_time,
             duration_t& last_idle_time) {

        _mutex.lock();
        scheduler_visit_count = _scheduler_visit_count;
        total_useful_time = _total_useful_time;
        total_retry_time = _total_retry_time;
        total_scheduler_time = _total_scheduler_time;
        total_idle_time = _total_idle_time;
        last_useful_time = _last_useful_time;
        last_retry_time = _last_retry_time;
        last_scheduler_time = _last_scheduler_time;
        last_idle_time = _last_idle_time;
        _mutex.unlock();
    }

};


#endif //JANA2_JWORKERMETRICS_H
