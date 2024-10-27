
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <cassert>
#include <vector>

#include "JArrowMetrics.h"
#include <JANA/JLogger.h>
#include <JANA/JException.h>
#include <JANA/Topology/JMailbox.h>
#include <JANA/Topology/JEventPool.h>


struct Place;
using JEventQueue = JMailbox<JEvent*>;


class JArrow {
    friend class JScheduler;
    friend class JTopologyBuilder;

public:
    using OutputData = std::array<std::pair<JEvent*, int>, 2>;

private:
    std::string m_name;        // Used for human understanding
    bool m_is_parallel;        // Whether or not it is safe to parallelize
    bool m_is_source;          // Whether or not this arrow should activate/drain the topology
    bool m_is_sink;            // Whether or not tnis arrow contributes to the final event count
    JArrowMetrics m_metrics;   // Performance information accumulated over all workers

    std::vector<JArrow *> m_listeners;    // Downstream Arrows

protected:
    JLogger m_logger;
    std::vector<Place*> m_places;  // Will eventually supplant m_listeners

public:
    std::string get_name() { return m_name; }
    JLogger& get_logger() { return m_logger; }
    bool is_parallel() { return m_is_parallel; }
    bool is_source() { return m_is_source; }
    bool is_sink() { return m_is_sink; }
    JArrowMetrics& get_metrics() { return m_metrics; }

    void set_name(std::string name) { m_name = name; }
    void set_logger(JLogger logger) { m_logger = logger; }
    void set_is_parallel(bool is_parallel) { m_is_parallel = is_parallel; }
    void set_is_source(bool is_source) { m_is_source = is_source; }
    void set_is_sink(bool is_sink) { m_is_sink = is_sink; }


    JArrow() {
        m_is_parallel = false;
        m_is_source = false;
        m_is_sink = false;
    }

    JArrow(std::string name, bool is_parallel, bool is_source, bool is_sink) :
            m_name(std::move(name)), m_is_parallel(is_parallel), m_is_source(is_source), m_is_sink(is_sink) {
    };

    virtual ~JArrow() = default;

    virtual void initialize() { };

    virtual void execute(JArrowMetrics& result, size_t location_id) = 0;

    virtual void finalize() {};

    // TODO: Make no longer virtual
    virtual size_t get_pending();

    void attach(JArrow* downstream) {
        m_listeners.push_back(downstream);
    };

    void attach(Place* place) {
        if (std::find(m_places.begin(), m_places.end(), place) == m_places.end()) {
            m_places.push_back(place);
        }
    };

    void attach(JMailbox<JEvent*>* queue, size_t port);
    void attach(JEventPool* pool, size_t port);

    JEvent* pull(size_t input_port, size_t location_id);
    void push(OutputData& outputs, size_t output_count, size_t location_id);
};


struct Place {
    void* place_ref = nullptr;
    bool is_queue = true;
    bool is_input = false;

    Place(JArrow* parent, bool is_input) {
        assert(parent != nullptr);
        parent->attach(this);
        this->is_input = is_input;
    }
};

inline size_t JArrow::get_pending() { 
    size_t sum = 0;
    for (Place* place : m_places) {
        if (place->is_input && place->is_queue) {
            auto queue = static_cast<JMailbox<JEvent*>*>(place->place_ref);
            sum += queue->size();
        }
    }
    return sum;
}


