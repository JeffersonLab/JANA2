
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef GREENFIELD_ARROW_H
#define GREENFIELD_ARROW_H

#include <iostream>
#include <assert.h>

#include "JActivable.h"
#include "JArrowMetrics.h"

class JArrow : public JActivable {

public:
    enum class NodeType {Source, Sink, Stage, Group};
    enum class BackoffStrategy { Constant, Linear, Exponential };
    using duration_t = std::chrono::steady_clock::duration;

private:
    // Info
    const std::string m_name;     // Used for human understanding
    const bool m_is_parallel;     // Whether or not it is safe to parallelize
    const NodeType m_type;

    // Statuses
    JArrowMetrics m_metrics;      // Performance information accumulated over all workers
    size_t m_thread_count = 0;    // Current number of threads assigned to this arrow
    std::atomic_bool m_is_upstream_finished {false };  // TODO: Deprecated. Use m_status instead.
    //Status m_status = Status::Unopened;  // Lives in JActivable for now

    // Knobs
    size_t m_chunksize = 1;       // Number of items to pop off the input queue at once
    BackoffStrategy m_backoff_strategy = BackoffStrategy::Exponential;
    duration_t m_initial_backoff_time = std::chrono::microseconds(1);
    duration_t m_checkin_time = std::chrono::milliseconds(500);
    unsigned m_backoff_tries = 4;

    mutable std::mutex m_mutex;   // Protects access to arrow properties.
                                 // TODO: Consider storing and protect thread count differently,
                                 // so that (number of workers) = (sum of thread counts for all arrows)
                                 // This is not so simple if we also want our WorkerStatus::arrow_name to match
public:

    // Constants

    bool is_parallel() { return m_is_parallel; }

    std::string get_name() { return m_name; }


public:

    // Written externally

    void set_chunksize(size_t chunksize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_chunksize = chunksize;
    }

    size_t get_chunksize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_chunksize;
    }

    void set_backoff_tries(unsigned backoff_tries) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_backoff_tries = backoff_tries;
    }

    unsigned get_backoff_tries() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_backoff_tries;
    }

    BackoffStrategy get_backoff_strategy() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_backoff_strategy;
    }

    void set_backoff_strategy(BackoffStrategy backoff_strategy) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_backoff_strategy = backoff_strategy;
    }

    duration_t get_initial_backoff_time() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_initial_backoff_time;
    }

    void set_initial_backoff_time(const duration_t& initial_backoff_time) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_initial_backoff_time = initial_backoff_time;
    }

    const duration_t& get_checkin_time() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_checkin_time;
    }

    void set_checkin_time(const duration_t& checkin_time) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_checkin_time = checkin_time;
    }

    void update_thread_count(int thread_count_delta) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_thread_count += thread_count_delta;
    }

    size_t get_thread_count() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_thread_count;
    }

    // TODO: Metrics should be encapsulated so that only actions are to update, clear, or summarize
    JArrowMetrics& get_metrics() {
        return m_metrics;
    }

    NodeType get_type() {
        return m_type;
    }

    JArrow(std::string name, bool is_parallel, NodeType arrow_type, size_t chunksize=16) :
            m_name(std::move(name)), m_is_parallel(is_parallel), m_type(arrow_type), m_chunksize(chunksize) {

        m_metrics.clear();
    };

    virtual ~JArrow() = default;

    virtual void initialize() {
        assert(get_status() == Status::Unopened);
    };

    virtual void execute(JArrowMetrics& result, size_t location_id) = 0;

    virtual void finalize() {
        assert(get_status() == Status::Stopped || get_status() == Status::Running);
    };


    virtual size_t get_pending() { return 0; }

    virtual size_t get_threshold() { return 0; }

    virtual void set_threshold(size_t /* threshold */) {}

protected:
    void on_status_change(JActivable::Status old_status, JActivable::Status new_status ) override {
        if (old_status == JActivable::Status::Unopened) {
            initialize();
        }
        else if (new_status == JActivable::Status::Finished) {
            finalize();
        }
    }
};


inline std::ostream& operator<<(std::ostream& os, const JArrow::NodeType& nt) {
    switch (nt) {
        case JArrow::NodeType::Stage: os << "Stage"; break;
        case JArrow::NodeType::Source: os << "Source"; break;
        case JArrow::NodeType::Sink: os << "Sink"; break;
        case JArrow::NodeType::Group: os << "Group"; break;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const JArrow::Status& s) {
    switch (s) {
        case JArrow::Status::Unopened: os << "Unopened"; break;
        case JArrow::Status::Running:  os << "Running"; break;
        case JArrow::Status::Stopped: os << "Stopped"; break;
        case JArrow::Status::Finished: os << "Finished"; break;
    }
    return os;
}

#endif // GREENFIELD_ARROW_H
