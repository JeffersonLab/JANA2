
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
        std::vector<std::shared_ptr<JEvent>*> available_items;
        std::vector<std::shared_ptr<JEvent>> items;
    };

    std::unique_ptr<LocalPool[]> m_pools;

    size_t m_pool_size;
    size_t m_location_count;

    std::shared_ptr<JComponentManager> m_component_manager;
    JEventLevel m_level;


public:
    inline JEventPool(std::shared_ptr<JComponentManager> component_manager,
                      size_t pool_size,
                      size_t location_count,
                      JEventLevel level = JEventLevel::PhysicsEvent)

            : m_pool_size(pool_size)
            , m_location_count(location_count)
            , m_component_manager(component_manager)
            , m_level(level) {

        assert(m_location_count >= 1);

        m_pools = std::unique_ptr<LocalPool[]>(new LocalPool[m_location_count]());

        for (size_t j=0; j<m_location_count; ++j) {

            m_pools[j].items = std::vector<std::shared_ptr<JEvent>>(m_pool_size); // Default-construct everything in place

            for (auto& item : m_pools[j].items) {
                configure_item(&item);
                m_pools[j].available_items.push_back(&item);
            }
        }
    }

    void configure_item(std::shared_ptr<JEvent>* item) {
        (*item) = std::make_shared<JEvent>();
        m_component_manager->configure_event(**item);
        item->get()->SetLevel(m_level); // This needs to happen _after_ configure_event
    }


    void finalize() {
        for (size_t pool_idx = 0; pool_idx < m_location_count; ++pool_idx) {
            for (auto& event : m_pools[pool_idx].items) {
                event->Finish();
            }
        }
    }

    std::shared_ptr<JEvent>* get(size_t location=0) {

        assert(m_pools != nullptr); // If you hit this, you forgot to call init().
        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);

        if (pool.available_items.empty()) {
            return nullptr;
        }
        else {
            std::shared_ptr<JEvent>* item = pool.available_items.back();
            pool.available_items.pop_back();
            return item;
        }
    }


    void put(std::shared_ptr<JEvent>* item, bool clear_event, size_t location) {

        assert(m_pools != nullptr); // If you hit this, you forgot to call init().
        
        if (clear_event) {
            // Do any necessary teardown within the item itself
            (*item)->Clear();
        }

        // Consider each location starting with current one
        for (size_t l = location; l<location+m_location_count; ++l) {
            LocalPool& pool = m_pools[l % m_location_count];

            // Check if item came from this location
            if (pool.available_items.size() < m_pool_size) {
                std::lock_guard<std::mutex> lock(pool.mutex);
                pool.available_items.push_back(item);
                return;
            }
        }
        throw JException("Event returned to already-full pool");
    }


    size_t pop(std::shared_ptr<JEvent>** dest, size_t min_count, size_t max_count, size_t location) {

        assert(m_pools != nullptr); // If you hit this, you forgot to call init().

        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);

        size_t available_count = pool.available_items.size();

        if (available_count < min_count) {
            // Exit immmediately if we can't reach the minimum
            return 0;
        }
        // Return as many as we can. We aren't allowed to create any more
        size_t count = std::min(available_count, max_count);
        for (size_t i=0; i<count; ++i) {
            std::shared_ptr<JEvent>* t = pool.available_items.back();
            pool.available_items.pop_back();
            dest[i] = t;
        }
        return count;
    }

    void push(std::shared_ptr<JEvent>** source, size_t count, bool clear_event, size_t location) {
        for (size_t i=0; i<count; ++i) {
            put(source[i], clear_event, location);
            source[i] = nullptr;
        }
    }
};


