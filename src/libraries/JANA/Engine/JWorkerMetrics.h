
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

public:
    using clock_t = std::chrono::steady_clock;
    using duration_t = clock_t::duration;
    using time_point_t = clock_t::time_point;

private:
    mutable std::mutex m_mutex;
    // mutex is mutable so that we can lock before reading from a const ref

    time_point_t m_last_heartbeat;
    long m_scheduler_visit_count;

    duration_t m_total_useful_time;
    duration_t m_total_retry_time;
    duration_t m_total_scheduler_time;
    duration_t m_total_idle_time;
    duration_t m_last_useful_time;
    duration_t m_last_retry_time;
    duration_t m_last_scheduler_time;
    duration_t m_last_idle_time;


public:
    JWorkerMetrics() {
        m_mutex.lock();
        m_last_heartbeat = clock_t::now();
        m_scheduler_visit_count = 0;
        auto zero = duration_t::zero();
        m_total_useful_time = zero;
        m_total_retry_time = zero;
        m_total_scheduler_time = zero;
        m_total_idle_time = zero;
        m_last_useful_time = zero;
        m_last_retry_time = zero;
        m_last_scheduler_time = zero;
        m_last_idle_time = zero;
        m_mutex.unlock();
    }

    void clear() {
        m_mutex.lock();
        m_last_heartbeat = clock_t::now();
        m_scheduler_visit_count = 0;
        auto zero = duration_t::zero();
        m_total_useful_time = zero;
        m_total_retry_time = zero;
        m_total_scheduler_time = zero;
        m_total_idle_time = zero;
        m_last_useful_time = zero;
        m_last_retry_time = zero;
        m_last_scheduler_time = zero;
        m_last_idle_time = zero;
        m_mutex.unlock();
    }


    void update(const JWorkerMetrics &other) {

        m_mutex.lock();
        other.m_mutex.lock();
        m_last_heartbeat = other.m_last_heartbeat;
        m_scheduler_visit_count += other.m_scheduler_visit_count;
        m_total_useful_time += other.m_total_useful_time;
        m_total_retry_time += other.m_total_retry_time;
        m_total_scheduler_time += other.m_total_scheduler_time;
        m_total_idle_time += other.m_total_idle_time;
        m_last_useful_time = other.m_last_useful_time;
        m_last_retry_time = other.m_last_retry_time;
        m_last_scheduler_time = other.m_last_scheduler_time;
        m_last_idle_time = other.m_last_idle_time;
        other.m_mutex.unlock();
        m_mutex.unlock();
    }


    void update(
                const time_point_t& heartbeat,
                const long& scheduler_visit_count,
                const duration_t& useful_time,
                const duration_t& retry_time,
                const duration_t& scheduler_time,
                const duration_t& idle_time) {

        m_mutex.lock();
        m_scheduler_visit_count += scheduler_visit_count;
        m_total_useful_time += useful_time;
        m_total_retry_time += retry_time;
        m_total_scheduler_time += scheduler_time;
        m_total_idle_time += idle_time;
        m_last_useful_time = useful_time;
        m_last_retry_time = retry_time;
        m_last_scheduler_time = scheduler_time;
        m_last_idle_time = idle_time;
        m_last_heartbeat = heartbeat;
        m_mutex.unlock();
    }


    void get(
             time_point_t& last_heartbeat,
             long& scheduler_visit_count,
             duration_t& total_useful_time,
             duration_t& total_retry_time,
             duration_t& total_scheduler_time,
             duration_t& total_idle_time,
             duration_t& last_useful_time,
             duration_t& last_retry_time,
             duration_t& last_scheduler_time,
             duration_t& last_idle_time) {

        m_mutex.lock();
        scheduler_visit_count = m_scheduler_visit_count;
        total_useful_time = m_total_useful_time;
        total_retry_time = m_total_retry_time;
        total_scheduler_time = m_total_scheduler_time;
        total_idle_time = m_total_idle_time;
        last_useful_time = m_last_useful_time;
        last_retry_time = m_last_retry_time;
        last_scheduler_time = m_last_scheduler_time;
        last_idle_time = m_last_idle_time;
        last_heartbeat = m_last_heartbeat;
        m_mutex.unlock();
    }

};


#endif //JANA2_JWORKERMETRICS_H
