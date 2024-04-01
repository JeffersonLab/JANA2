
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

struct PlaceRefBase;

class JArrow {
private:
    const std::string m_name;     // Used for human understanding
    const bool m_is_parallel;     // Whether or not it is safe to parallelize
    const bool m_is_source;       // Whether or not this arrow should activate/drain the topology
    bool m_is_sink;         // Whether or not tnis arrow contributes to the final event count
    JArrowMetrics m_metrics;      // Performance information accumulated over all workers

    mutable std::mutex m_arrow_mutex;  // Protects access to arrow properties

    // TODO: Get rid of me
    size_t m_chunksize = 1;       // Number of items to pop off the input queue at once

    friend class JScheduler;
    std::vector<JArrow *> m_listeners;    // Downstream Arrows
    friend class JTopologyBuilder;
    std::vector<PlaceRefBase*> m_places;  // Will eventually supplant m_listeners, m_chunksize

protected:
    // This is usable by subclasses.
    JLogger m_logger;

public:
    bool is_parallel() { return m_is_parallel; }
    bool is_source() { return m_is_source; }
    bool is_sink() { return m_is_sink; }

    std::string get_name() { return m_name; }

    void set_logger(JLogger logger) {
        m_logger = logger;
    }

    void set_is_sink(bool is_sink) {
        m_is_sink = is_sink;
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
    virtual size_t get_pending();

    // TODO: Get rid of me
    virtual size_t get_threshold();

    virtual void set_threshold(size_t /* threshold */);

    void attach(JArrow* downstream) {
        m_listeners.push_back(downstream);
    };

    void attach(PlaceRefBase* place) {
        if (std::find(m_places.begin(), m_places.end(), place) == m_places.end()) {
            m_places.push_back(place);
        }
    };
};

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

struct PlaceRefBase {
    void* place_ref = nullptr;
    bool is_queue = true;
    bool is_input = false;
    size_t min_item_count = 1;
    size_t max_item_count = 1;

    virtual size_t get_pending() { return 0; }
    virtual size_t get_threshold() { return 0; }
    virtual void set_threshold(size_t) {}
};

template <typename T>
struct PlaceRef : public PlaceRefBase {

    PlaceRef(JArrow* parent) {
        assert(parent != nullptr);
        parent->attach(this);
    }

    PlaceRef(JArrow* parent, bool is_input, size_t min_item_count, size_t max_item_count) {
        assert(parent != nullptr);
        parent->attach(this);
        this->is_input = is_input;
        this->min_item_count = min_item_count;
        this->max_item_count = max_item_count;
    }

    PlaceRef(JArrow* parent, JMailbox<T*>* queue, bool is_input, size_t min_item_count, size_t max_item_count) {
        assert(parent != nullptr);
        assert(queue != nullptr);
        parent->attach(this);
        this->place_ref = queue;
        this->is_queue = true;
        this->is_input = is_input;
        this->min_item_count = min_item_count;
        this->max_item_count = max_item_count;
    }

    PlaceRef(JArrow* parent, JPool<T>* pool, bool is_input, size_t min_item_count, size_t max_item_count)  {
        assert(parent != nullptr);
        assert(pool != nullptr);
        parent->attach(this);
        this->place_ref = pool;
        this->is_queue = false;
        this->is_input = is_input;
        this->min_item_count = min_item_count;
        this->max_item_count = max_item_count;
    }

    void set_queue(JMailbox<T*>* queue) {
        assert(queue != nullptr);
        this->place_ref = queue;
        this->is_queue = true;
    }

    void set_pool(JPool<T>* pool) {
        assert(pool != nullptr);
        this->place_ref = pool;
        this->is_queue = false;
    }

    size_t get_pending() override {
        assert(place_ref != nullptr);
        if (is_input && is_queue) {
            auto queue = static_cast<JMailbox<T*>*>(place_ref);
            return queue->size();
        }
        return 0;
    }

    size_t get_threshold() override {
        assert(place_ref != nullptr);
        if (is_input && is_queue) {
            auto queue = static_cast<JMailbox<T*>*>(place_ref);
            return queue->get_threshold();
        }
        return -1;
    }

    void set_threshold(size_t threshold) override {
        assert(place_ref != nullptr);
        if (is_input && is_queue) {
            auto queue = static_cast<JMailbox<T*>*>(place_ref);
            queue->set_threshold(threshold);
        }
    }

    bool pull(Data<T>& data) {
        assert(place_ref != nullptr);
        if (is_input) { // Actually pull the data
            if (is_queue) {
                auto queue = static_cast<JMailbox<T*>*>(place_ref);
                data.item_count = queue->pop_and_reserve(data.items.data(), min_item_count, max_item_count, data.location_id);
                data.reserve_count = data.item_count;
                return (data.item_count >= min_item_count);
            }
            else {
                auto pool = static_cast<JPool<T>*>(place_ref);
                data.item_count = pool->pop(data.items.data(), min_item_count, max_item_count, data.location_id);
                data.reserve_count = 0;
                return (data.item_count >= min_item_count);
            }
        }
        else {
            if (is_queue) {
                // Reserve a space on the output queue
                data.item_count = 0;
                auto queue = static_cast<JMailbox<T*>*>(place_ref);
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
        assert(place_ref != nullptr);
        if (is_queue) {
            auto queue = static_cast<JMailbox<T*>*>(place_ref);
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
        }
        else {
            if (is_input) {
                auto pool = static_cast<JPool<T>*>(place_ref);
                pool->push(data.items.data(), data.item_count, data.location_id);
            }
        }
    }

    size_t push(Data<T>& data) {
        assert(place_ref != nullptr);
        if (is_queue) {
            auto queue = static_cast<JMailbox<T*>*>(place_ref);
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
            data.item_count = 0;
            data.reserve_count = 0;
            return is_input ? 0 : data.item_count;
        }
        else {
            auto pool = static_cast<JPool<T>*>(place_ref);
            pool->push(data.items.data(), data.item_count, data.location_id);
            data.item_count = 0;
            data.reserve_count = 0;
            return 1;
        }
    }
};

inline size_t JArrow::get_pending() { 
    size_t sum = 0;
    for (PlaceRefBase* place : m_places) {
        sum += place->get_pending();
    }
    return sum;
}

inline size_t JArrow::get_threshold() {
    size_t result = -1;
    for (PlaceRefBase* place : m_places) {
        result = std::min(result, place->get_threshold());
    }
    return result;

}

inline void JArrow::set_threshold(size_t threshold) {
    for (PlaceRefBase* place : m_places) {
        place->set_threshold(threshold);
    }
}


#endif // GREENFIELD_ARROW_H
