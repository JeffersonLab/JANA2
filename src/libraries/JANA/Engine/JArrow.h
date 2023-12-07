
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
#include <JANA/Engine/JMailbox.h>
#include <JANA/Engine/JPool.h>


#ifndef JANA2_ARROWDATA_MAX_SIZE
#define JANA2_ARROWDATA_MAX_SIZE 10
#endif

template <typename T>
struct Data {
    std::array<T*, JANA2_ARROWDATA_MAX_SIZE> items;
    size_t item_count = 0;
    size_t reserve_count = 0;
    size_t location_id;

    Data(size_t location_id = 0) : location_id(location_id) {
        items = {nullptr};
    }
};

template <typename T>
struct PlaceRef {
    JMailbox<T*>* queue = nullptr;
    JPool<T>* pool = nullptr;
    bool is_input = false;
    size_t min_item_count = 1;
    size_t max_item_count = 1;

    PlaceRef() = default;

    PlaceRef(JMailbox<T*>* queue, bool is_input, size_t min_item_count, size_t max_item_count) 
        : queue(queue), is_input(is_input), 
          min_item_count(min_item_count), max_item_count(max_item_count) {}

    PlaceRef(JPool<T>* pool, bool is_input, size_t min_item_count, size_t max_item_count) 
        : pool(pool), is_input(is_input), 
          min_item_count(min_item_count), max_item_count(max_item_count) {}


    bool pull(Data<T>& data) {
        if (is_input) { // Actually pull the data
            if (queue != nullptr) {
                data.item_count = queue->pop_and_reserve(data.items.data(), min_item_count, max_item_count, data.location_id);
                data.reserve_count = data.item_count;
                return (data.item_count >= min_item_count);
            }
            else {
                data.item_count = pool->pop(data.items.data(), min_item_count, max_item_count, data.location_id);
                data.reserve_count = 0;
                return (data.item_count >= min_item_count);
            }
        }
        else {
            if (queue != nullptr) {
                // Reserve a space on the output queue
                data.item_count = 0;
                data.reserve_count = queue->reserve(min_item_count, max_item_count, data.location_id);
                return (data.reserve_count >= min_item_count);
            }
            else {
                // No need to reserve on pool -- either there is space or limit_events_in_flight=false
                data.item_count = 0;
                data.reserve_count = 0;
                return true;
            }
        }
    }

    void revert(Data<T>& data) {
        if (queue != nullptr) {
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
        }
        else {
            if (is_input) {
                pool->push(data.items.data(), data.item_count, data.location_id);
            }
        }
    }

    size_t push(Data<T>& data) {
        if (queue != nullptr) {
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
            data.item_count = 0;
            data.reserve_count = 0;
            return is_input ? 0 : data.item_count;
        }
        else {
            pool->push(data.items.data(), data.item_count, data.location_id);
            data.item_count = 0;
            data.reserve_count = 0;
            return 1;
        }
    }
};



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

    // TODO: Get rid of me
    void set_chunksize(size_t chunksize) {
        std::lock_guard<std::mutex> lock(m_arrow_mutex);
        m_chunksize = chunksize;
    }

    // TODO: Get rid of me
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

    // TODO: Make no longer virtual
    virtual size_t get_pending() { return 0; }

    // TODO: Get rid of me
    virtual size_t get_threshold() { return 0; }

    virtual void set_threshold(size_t /* threshold */) {}

    void attach(JArrow* downstream) {
        m_listeners.push_back(downstream);
    };

};


#endif // GREENFIELD_ARROW_H
