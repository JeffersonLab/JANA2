
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include <JANA/JEvent.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/JMultifactory.h>
#include <mutex>
#include <vector>


class JEventPool {
private:
    struct alignas(JANA2_CACHE_LINE_BYTES) LocalPool {
        std::mutex mutex;
        std::vector<JEvent*> available_items;
    };

    std::vector<std::unique_ptr<LocalPool>> m_pools;
    std::vector<std::shared_ptr<JEvent>> m_owned_events;

    size_t m_max_inflight_events;
    size_t m_location_count;

    std::shared_ptr<JComponentManager> m_component_manager;
    JEventLevel m_level;
    JLogger m_logger;


public:
    inline JEventPool(std::shared_ptr<JComponentManager> component_manager,
                      size_t max_inflight_events,
                      size_t location_count,
                      JEventLevel level = JEventLevel::PhysicsEvent)

            : m_max_inflight_events(max_inflight_events)
            , m_location_count(location_count)
            , m_component_manager(component_manager)
            , m_level(level) {

        assert(m_location_count >= 1);

        // Create LocalPools for each location
        for (size_t loc=0; loc<m_location_count; ++loc) {
            auto pool = std::make_unique<LocalPool>();
            pool->available_items.reserve(max_inflight_events);
            m_pools.push_back(std::move(pool));
        }

        // Create JEvents and distribute them among the pools
        m_owned_events.reserve(max_inflight_events);
        for (size_t evt_idx=0; evt_idx<max_inflight_events; evt_idx++) {

            m_owned_events.push_back(std::make_shared<JEvent>());
            auto evt = &m_owned_events.back(); 
            m_component_manager->configure_event(**evt);
            (*evt)->SetLevel(m_level);
            m_pools[evt_idx % m_location_count]->available_items.push_back(evt->get());
        }
    }


    void finalize() {
        for (auto& evt : m_owned_events) {
            evt->Finish();
        }
    }


    JEvent* get(size_t location=0) {

        // Note: For now this doesn't steal from another pool. In principle this means that
        // all of the JEvents could end up in a single location's pool and there would be no way for
        // the JEventSource in other locations to obtain fresh events. I don't think this is a problem
        // in practice because the arrows always push and pull to pool's location 0, but we should 
        // revisit this when the time is right.

        LocalPool& pool = *(m_pools[location % m_location_count]);
        std::lock_guard<std::mutex> lock(pool.mutex);

        if (pool.available_items.empty()) {
            return nullptr;
        }
        else {
            JEvent* item = pool.available_items.back();
            pool.available_items.pop_back();
            return item;
        }
    }


    void put(JEvent* item, bool clear_event, size_t location) {

        if (clear_event) {
            // Do any necessary teardown within the item itself
            item->Clear();
        }
        auto use_count = item->shared_from_this().use_count();

        LocalPool& pool = *(m_pools[location]);

        if (pool.available_items.size() > m_max_inflight_events) {
            throw JException("Attempted to return a JEvent to an already-full pool");
        }

        std::lock_guard<std::mutex> lock(pool.mutex);
        pool.available_items.push_back(item);
    }


    size_t pop(JEvent** dest, size_t min_count, size_t max_count, size_t location) {

        LocalPool& pool = *(m_pools[location % m_location_count]);
        std::lock_guard<std::mutex> lock(pool.mutex);

        size_t available_count = pool.available_items.size();

        if (available_count < min_count) {
            // Exit immmediately if we can't reach the minimum
            return 0;
        }
        // Return as many as we can. We aren't allowed to create any more
        size_t count = std::min(available_count, max_count);
        for (size_t i=0; i<count; ++i) {
            JEvent* t = pool.available_items.back();
            pool.available_items.pop_back();
            dest[i] = t;
        }
        return count;
    }

    void push(JEvent** source, size_t count, bool clear_event, size_t location) {
        for (size_t i=0; i<count; ++i) {
            put(source[i], clear_event, location);
            source[i] = nullptr;
        }
    }
};


