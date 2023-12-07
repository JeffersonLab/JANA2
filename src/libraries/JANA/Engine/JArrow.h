
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef GREENFIELD_ARROW_H
#define GREENFIELD_ARROW_H

#include <iostream>
#include <atomic>
#include <cassert>
#include <vector>

#include "JArrowMetrics.h"
#include <JANA/JLogger.h>
#include <JANA/JException.h>

class JArrow {
private:
    // Info
    const std::string m_name;     // Used for human understanding
    const bool m_is_parallel;     // Whether or not it is safe to parallelize
    const bool m_is_source;       // Whether or not this arrow should activate/drain the topology
    const bool m_is_sink;         // Whether or not tnis arrow contributes to the final event count
    JArrowMetrics m_metrics;      // Performance information accumulated over all workers

    mutable std::mutex m_arrow_mutex;  // Protects access to arrow properties

    // Knobs
    size_t m_chunksize = 1;       // Number of items to pop off the input queue at once

    friend class JScheduler;
    std::vector<JArrow *> m_listeners;     // Downstream Arrows

protected:
    // This is usable by subclasses.
    // Note that it has to be injected because JArrow doesn't know about JApplication, etc
    JLogger m_logger {JLogger::Level::OFF};

public:

    // Constants

    bool is_parallel() { return m_is_parallel; }
    bool is_source() { return m_is_source; }
    bool is_sink() { return m_is_sink; }

    std::string get_name() { return m_name; }


    // Written externally

    void set_logger(JLogger logger) {
        m_logger = logger;
    }

    void set_chunksize(size_t chunksize) {
        std::lock_guard<std::mutex> lock(m_arrow_mutex);
        m_chunksize = chunksize;
    }

    size_t get_chunksize() const {
        std::lock_guard<std::mutex> lock(m_arrow_mutex);
        return m_chunksize;
    }


    // TODO: Metrics should be encapsulated so that only actions are to update, clear, or summarize
    JArrowMetrics& get_metrics() {
        return m_metrics;
    }

    JArrow(std::string name, bool is_parallel, bool is_source, bool is_sink, size_t chunksize=16) :
            m_name(std::move(name)), m_is_parallel(is_parallel), m_is_source(is_source), m_is_sink(is_sink), m_chunksize(chunksize) {

        m_metrics.clear();
    };

    virtual ~JArrow() = default;

    virtual void initialize() { };

    virtual void execute(JArrowMetrics& result, size_t location_id) = 0;

    virtual void finalize() {};

    virtual size_t get_pending() { return 0; }

    virtual size_t get_threshold() { return 0; }

    virtual void set_threshold(size_t /* threshold */) {}


    void attach(JArrow* downstream) {
        m_listeners.push_back(downstream);
    };

};


#endif // GREENFIELD_ARROW_H
