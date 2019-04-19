//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_ARROW_H
#define GREENFIELD_ARROW_H

#include <vector>

#include <JANA/Queue.h>
#include "JServiceLocator.h"
#include "JArrowMetrics.h"


using duration_t = std::chrono::steady_clock::duration;

class JArrow : public JActivable {


private:
    const std::string _name;     // Used for human understanding
    const bool _is_parallel;     // Whether or not it is safe to parallelize
    JArrowMetrics _metrics;      // Performance information accumulated over all workers
    size_t _chunksize = 1;       // Number of items to pop off the input queue at once
    size_t _thread_count = 0;    // Current number of threads assigned to this arrow
    bool _is_upstream_finished = false;

    std::mutex _mutex;           // Protects access to arrow properties.
                                 // TODO: Consider storing and protect thread count differently,
                                 // so that (number of workers) = (sum of thread counts for all arrows)
                                 // This is not so simple if we also want our WorkerStatus::arrow_name to match


public:

    // TODO: Add NoWork, BackPressure

    // Constants

    bool is_parallel() { return _is_parallel; }

    std::string get_name() { return _name; }

    // Written internally, read externally

    bool is_upstream_finished() { return _is_upstream_finished; }


protected:

    // Written internally, read externally

    void set_upstream_finished(bool upstream_finished) { _is_upstream_finished = upstream_finished; }


public:

    // Written externally

    void set_chunksize(size_t chunksize) {
        std::lock_guard<std::mutex> lock(_mutex);
        _chunksize = chunksize;
    }

    size_t get_chunksize() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _chunksize;
    }

    void update_thread_count(size_t thread_count_delta) {
        std::lock_guard<std::mutex> lock(_mutex);
        _thread_count += thread_count_delta;
    }

    size_t get_thread_count() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _thread_count;
    }

    JArrowMetrics& get_metrics() {
        return _metrics;
    }

    JArrow(std::string name, bool is_parallel) :
            _name(std::move(name)), _is_parallel(is_parallel) {

        if (serviceLocator != nullptr) {
            auto params = serviceLocator->get<ParameterManager>();
            _chunksize = params->chunksize;
        }
        _metrics.clear();
    };

    virtual ~JArrow() = default;

    virtual void execute(JArrowMetrics& result) = 0;

};





#endif // GREENFIELD_ARROW_H
