
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JARROWMETRIC_H
#define JANA2_JARROWMETRIC_H

#include <mutex>
#include <chrono>

class JArrowMetrics {

public:
    enum class Status {KeepGoing, ComeBackLater, Finished, NotRunYet, Error};
    using duration_t = std::chrono::steady_clock::duration;

private:
    mutable std::mutex m_mutex;
    // Mutex is mutable so that we can lock before reading from a const ref

    Status m_last_status;
    size_t m_total_message_count;
    size_t m_last_message_count;
    size_t m_total_queue_visits;
    size_t m_last_queue_visits;
    duration_t m_total_latency;
    duration_t m_last_latency;
    duration_t m_total_queue_latency;
    duration_t m_last_queue_latency;


    // TODO: We might want to add a timestamp, so that
    // the 'last_*' measurements can reflect the most recent value,
    // rather than the last-to-be-accumulated value.

public:
    void clear() {

        m_mutex.lock();
        m_last_status = Status::NotRunYet;
        m_total_message_count = 0;
        m_last_message_count = 0;
        m_total_queue_visits = 0;
        m_last_queue_visits = 0;
        m_total_latency = duration_t::zero();
        m_last_latency = duration_t::zero();
        m_total_queue_latency = duration_t::zero();
        m_last_queue_latency = duration_t::zero();
        m_mutex.unlock();
    }

    void take(JArrowMetrics& other) {

        m_mutex.lock();
        other.m_mutex.lock();

        if (other.m_last_message_count != 0) {
            m_last_message_count = other.m_last_message_count;
            m_last_latency = other.m_last_latency;
        }

        m_last_status = other.m_last_status;
        m_total_message_count += other.m_total_message_count;
        m_total_queue_visits += other.m_total_queue_visits;
        m_last_queue_visits = other.m_last_queue_visits;
        m_total_latency += other.m_total_latency;
        m_total_queue_latency += other.m_total_queue_latency;
        m_last_queue_latency = other.m_last_queue_latency;

        other.m_last_status = Status::NotRunYet;
        other.m_total_message_count = 0;
        other.m_last_message_count = 0;
        other.m_total_queue_visits = 0;
        other.m_last_queue_visits = 0;
        other.m_total_latency = duration_t::zero();
        other.m_last_latency = duration_t::zero();
        other.m_total_queue_latency = duration_t::zero();
        other.m_last_queue_latency = duration_t::zero();
        other.m_mutex.unlock();
        m_mutex.unlock();
    };

    void update(const JArrowMetrics &other) {

        m_mutex.lock();
        other.m_mutex.lock();

        if (other.m_last_message_count != 0) {
            m_last_message_count = other.m_last_message_count;
            m_last_latency = other.m_last_latency;
        }
        m_total_latency += other.m_total_latency;
        m_last_status = other.m_last_status;
        m_total_message_count += other.m_total_message_count;
        m_total_queue_visits += other.m_total_queue_visits;
        m_last_queue_visits = other.m_last_queue_visits;
        m_total_queue_latency += other.m_total_queue_latency;
        m_last_queue_latency = other.m_last_queue_latency;
        other.m_mutex.unlock();
        m_mutex.unlock();
    };

    void update_finished() {
        m_mutex.lock();
        m_last_status = Status::Finished;
        m_mutex.unlock();
    }

    void update(const Status& last_status,
                const size_t& message_count_delta,
                const size_t& queue_visit_delta,
                const duration_t& latency_delta,
                const duration_t& queue_latency_delta) {

        m_mutex.lock();
        m_last_status = last_status;

        if (message_count_delta > 0) {
            // We don't want to lose our most recent latency numbers
            // when the most recent execute() encounters an empty
            // queue and consequently processes zero items.
            m_last_message_count = message_count_delta;
            m_last_latency = latency_delta;
        }
        m_total_message_count += message_count_delta;
        m_total_queue_visits += queue_visit_delta;
        m_last_queue_visits = queue_visit_delta;
        m_total_latency += latency_delta;
        m_total_queue_latency += queue_latency_delta;
        m_last_queue_latency = queue_latency_delta;
        m_mutex.unlock();

    };

    void get(Status& last_status,
             size_t& total_message_count,
             size_t& last_message_count,
             size_t& total_queue_visits,
             size_t& last_queue_visits,
             duration_t& total_latency,
             duration_t& last_latency,
             duration_t& total_queue_latency,
             duration_t& last_queue_latency) {

        m_mutex.lock();
        last_status = m_last_status;
        total_message_count = m_total_message_count;
        last_message_count = m_last_message_count;
        total_queue_visits = m_total_queue_visits;
        last_queue_visits = m_last_queue_visits;
        total_latency = m_total_latency;
        last_latency = m_last_latency;
        total_queue_latency = m_total_queue_latency;
        last_queue_latency = m_last_queue_latency;
        m_mutex.unlock();
    }

    size_t get_total_message_count() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_total_message_count;
    }

    Status get_last_status() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_last_status;
    }

    void summarize() {

    }
};

inline std::string to_string(JArrowMetrics::Status h) {
    switch (h) {
        case JArrowMetrics::Status::KeepGoing:     return "KeepGoing";
        case JArrowMetrics::Status::ComeBackLater: return "ComeBackLater";
        case JArrowMetrics::Status::Finished:      return "Finished";
        case JArrowMetrics::Status::NotRunYet:     return "NotRunYet";
        case JArrowMetrics::Status::Error:
        default:                          return "Error";
    }
}

#endif //JANA2_JARROWMETRIC_H
