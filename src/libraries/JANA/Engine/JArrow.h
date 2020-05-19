//
// Created by nbrei on 3/25/19.
//

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
    const std::string _name;     // Used for human understanding
    const bool _is_parallel;     // Whether or not it is safe to parallelize
    const NodeType _type;

    // Statuses
    JArrowMetrics _metrics;      // Performance information accumulated over all workers
    size_t _thread_count = 0;    // Current number of threads assigned to this arrow
    std::atomic_bool _is_upstream_finished { false };  // TODO: Deprecated. Use _status instead.
    //Status _status = Status::Unopened;  // Lives in JActivable for now

    // Knobs
    size_t _chunksize = 1;       // Number of items to pop off the input queue at once
    BackoffStrategy _backoff_strategy = BackoffStrategy::Exponential;
    duration_t _initial_backoff_time = std::chrono::microseconds(10);
    duration_t _checkin_time = std::chrono::milliseconds(500);
    unsigned _backoff_tries = 4;

    mutable std::mutex _mutex;   // Protects access to arrow properties.
                                 // TODO: Consider storing and protect thread count differently,
                                 // so that (number of workers) = (sum of thread counts for all arrows)
                                 // This is not so simple if we also want our WorkerStatus::arrow_name to match
public:
public:

    // Constants

    bool is_parallel() { return _is_parallel; }

    std::string get_name() { return _name; }

    // Written internally, read externally

    bool is_upstream_finished() { return _is_upstream_finished; }


protected:

    // Written internally, read externally

    void set_upstream_finished(bool upstream_finished) { _is_upstream_finished = upstream_finished; }

    void set_status(Status status) { _status = status; }


public:

    // Written externally

    void set_chunksize(size_t chunksize) {
        std::lock_guard<std::mutex> lock(_mutex);
        _chunksize = chunksize;
    }

    size_t get_chunksize() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _chunksize;
    }

    void set_backoff_tries(unsigned backoff_tries) {
        std::lock_guard<std::mutex> lock(_mutex);
        _backoff_tries = backoff_tries;
    }

    unsigned get_backoff_tries() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _backoff_tries;
    }

    BackoffStrategy get_backoff_strategy() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _backoff_strategy;
    }

    void set_backoff_strategy(BackoffStrategy backoff_strategy) {
        std::lock_guard<std::mutex> lock(_mutex);
        _backoff_strategy = backoff_strategy;
    }

    duration_t get_initial_backoff_time() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _initial_backoff_time;
    }

    void set_initial_backoff_time(const duration_t& initial_backoff_time) {
        std::lock_guard<std::mutex> lock(_mutex);
        _initial_backoff_time = initial_backoff_time;
    }

    const duration_t& get_checkin_time() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _checkin_time;
    }

    void set_checkin_time(const duration_t& checkin_time) {
        std::lock_guard<std::mutex> lock(_mutex);
        _checkin_time = checkin_time;
    }

    void update_thread_count(int thread_count_delta) {
        std::lock_guard<std::mutex> lock(_mutex);
        _thread_count += thread_count_delta;
    }

    size_t get_thread_count() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _thread_count;
    }

    // TODO: Metrics should be encapsulated so that only actions are to update, clear, or summarize
    JArrowMetrics& get_metrics() {
        return _metrics;
    }

    Status get_status() {
        return _status;
    }

    NodeType get_type() {
        return _type;
    }

    JArrow(std::string name, bool is_parallel, NodeType arrow_type, size_t chunksize=16) :
            _name(std::move(name)), _is_parallel(is_parallel), _type(arrow_type), _chunksize(chunksize) {

        _metrics.clear();
    };

    virtual ~JArrow() = default;

    virtual void initialize() {
        assert(_status == Status::Unopened);
        _status = Status::Inactive;
    };

    virtual void execute(JArrowMetrics& result, size_t location_id) = 0;

    virtual void finalize() {
        _status = Status::Closed;
    };


    virtual size_t get_pending() { return 0; }

    virtual size_t get_threshold() { return 0; }

    virtual void set_threshold(size_t threshold) {}

    void set_active(bool is_active) override {
        if (is_active) {
            assert(_status != Status::Closed);
            if (_status == Status::Unopened) {
                initialize();
            }
            _status = Status::Running;
        }
        else {
            //assert(_status != Status::Unopened);
            _status = Status::Inactive;
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
        case JArrow::Status::Inactive: os << "Inactive"; break;
        case JArrow::Status::Running:  os << "Running"; break;
        case JArrow::Status::Draining: os << "Draining"; break;
        case JArrow::Status::Drained: os << "Drained"; break;
        case JArrow::Status::Finished: os << "Finished"; break;
        case JArrow::Status::Closed: os << "Closed"; break;
    }
    return os;
}

#endif // GREENFIELD_ARROW_H
