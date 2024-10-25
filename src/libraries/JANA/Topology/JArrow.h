
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


#ifndef JANA2_ARROWDATA_MAX_SIZE
#define JANA2_ARROWDATA_MAX_SIZE 10
#endif

struct Place;

class JArrow {
    friend class JScheduler;
    friend class JTopologyBuilder;

private:
    std::string m_name;        // Used for human understanding
    bool m_is_parallel;        // Whether or not it is safe to parallelize
    bool m_is_source;          // Whether or not this arrow should activate/drain the topology
    bool m_is_sink;            // Whether or not tnis arrow contributes to the final event count
    JArrowMetrics m_metrics;   // Performance information accumulated over all workers

    std::vector<JArrow *> m_listeners;    // Downstream Arrows

protected:
    // This is usable by subclasses.
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


    JArrow(std::string name, bool is_parallel, bool is_source, bool is_sink) :
            m_name(std::move(name)), m_is_parallel(is_parallel), m_is_source(is_source), m_is_sink(is_sink) {

        m_metrics.clear();
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
};

struct Data {
    std::array<JEvent*, JANA2_ARROWDATA_MAX_SIZE> items;
    size_t item_count = 0;
    size_t reserve_count = 0;
    size_t location_id;

    Data(size_t location_id = 0) : location_id(location_id) {
        items = {nullptr};
    }
};

struct Place {
    void* place_ref = nullptr;
    bool is_queue = true;
    bool is_input = false;
    size_t min_item_count = 1;
    size_t max_item_count = 1;

    Place(JArrow* parent, bool is_input, size_t min_item_count, size_t max_item_count) {
        assert(parent != nullptr);
        parent->attach(this);
        this->is_input = is_input;
        this->min_item_count = min_item_count;
        this->max_item_count = max_item_count;
    }

    void set_queue(JMailbox<JEvent*>* queue) {
        assert(queue != nullptr);
        this->place_ref = queue;
        this->is_queue = true;
    }

    void set_pool(JEventPool* pool) {
        assert(pool != nullptr);
        this->place_ref = pool;
        this->is_queue = false;
    }

    size_t get_pending() {
        assert(place_ref != nullptr);
        if (is_input && is_queue) {
            auto queue = static_cast<JMailbox<JEvent*>*>(place_ref);
            return queue->size();
        }
        return 0;
    }

    bool pull(Data& data) {
        assert(place_ref != nullptr);
        if (is_input) { // Actually pull the data
            if (is_queue) {
                auto queue = static_cast<JMailbox<JEvent*>*>(place_ref);
                data.item_count = queue->pop(data.items.data(), min_item_count, max_item_count, data.location_id);
                //data.item_count = queue->pop_and_reserve(data.items.data(), min_item_count, max_item_count, data.location_id);
                return (data.item_count >= min_item_count);
            }
            else {
                auto pool = static_cast<JEventPool*>(place_ref);
                data.item_count = pool->pop(data.items.data(), min_item_count, max_item_count, data.location_id);
                data.reserve_count = 0;
                return (data.item_count >= min_item_count);
            }
        }
        else {
            if (is_queue) {
                // Reserve a space on the output queue
                data.item_count = 0;
                //auto queue = static_cast<JMailbox<JEvent*>*>(place_ref);
                data.reserve_count = 0; //queue->reserve(min_item_count, max_item_count, data.location_id);
                return true; //(data.reserve_count >= min_item_count);
            }
            else {
                // No need to reserve on pool -- either there is space or limit_events_in_flight=false
                data.item_count = 0;
                data.reserve_count = 0;
                return true;
            }
        }
    }

    void revert(Data& data) {
        assert(place_ref != nullptr);
        if (is_queue) {
            auto queue = static_cast<JMailbox<JEvent*>*>(place_ref);
            assert(data.reserve_count == 0);
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
        }
        else {
            if (is_input) {
                auto pool = static_cast<JEventPool*>(place_ref);
                pool->push(data.items.data(), data.item_count, false, data.location_id);
            }
        }
    }

    size_t push(Data& data) {
        assert(place_ref != nullptr);
        if (is_queue) {
            auto queue = static_cast<JMailbox<JEvent*>*>(place_ref);
            assert(data.reserve_count == 0);
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
            data.item_count = 0;
            data.reserve_count = 0;
            return is_input ? 0 : data.item_count;
        }
        else {
            auto pool = static_cast<JEventPool*>(place_ref);
            pool->push(data.items.data(), data.item_count, !is_input, data.location_id);
            data.item_count = 0;
            data.reserve_count = 0;
            return 1;
        }
    }
};

inline size_t JArrow::get_pending() { 
    size_t sum = 0;
    for (Place* place : m_places) {
        sum += place->get_pending();
    }
    return sum;
}


