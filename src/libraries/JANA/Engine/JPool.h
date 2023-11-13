// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Utils/JCpuInfo.h>

template <typename T>
class JPool {
private:
    struct alignas(JANA2_CACHE_LINE_BYTES) LocalPool {
        std::mutex mutex;
        std::vector<T*> available_items;
        std::vector<T> items;
    };

    size_t m_pool_size;
    size_t m_location_count;
    bool m_limit_total_events_in_flight;
    std::unique_ptr<LocalPool[]> m_pools;
  
public:
    JPool(size_t pool_size,
                      size_t location_count,
                      bool limit_total_events_in_flight)
        : m_pool_size(pool_size)
        , m_location_count(location_count)
        , m_limit_total_events_in_flight(limit_total_events_in_flight)
    {
        assert(m_location_count >= 1);
        assert(m_pool_size > 0 || !m_limit_total_events_in_flight);
        m_pools = std::unique_ptr<LocalPool[]>(new LocalPool[location_count]());

        for (size_t j=0; j<m_location_count; ++j) {
            for (size_t i=0; i<m_pool_size; ++i) {
                m_pools[j].items.emplace_back();
                configure_item(&(m_pools[j].items[i]));
            }
        }
    }

    virtual void configure_item(T* item) {
    }

    virtual void release_item(T* item) {
    }


    T* get(size_t location=0) {

        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);

        if (pool.available_items.empty()) {
            if (m_limit_total_events_in_flight) {
                return nullptr;
            }
            else {
                auto t = new T;
                configure_item(t);
                return t;
            }
        }
        else {
            T* item = pool.available_items.back();
            pool.available_items.pop_back();
            return item;
        }
    }


    void put(T* item, size_t location=0) {

        // TODO: Try to return item to original LocalPool
        release_item(item);
        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);

        if (pool.available_items.size() < m_pool_size) {
            pool.available_items.push_back(item);
        }
        else {
            delete item;
        }
    }

    size_t size() { return m_pool_size; }


    bool get_many(std::vector<T*>& dest, size_t count, size_t location=0) {

        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);
        // TODO: We probably want to steal from other event pools if jana:enable_stealing=true

        if (m_limit_total_events_in_flight && pool.available_items.size() < count) {
            return false;
        }
        else {
            while (count > 0 && !pool.available_items.empty()) {
                T* t = pool.available_items.back();
                pool.available_items.pop_back();
                dest.push_back(t);
                count -= 1;
            }
            while (count > 0) {
                auto t = new T;
                configure_item(t);
                dest.push_back(t);
                count -= 1;
            }
            return true;
        }
    }

    void put_many(std::vector<std::shared_ptr<JEvent>>& finished_events, size_t location=0) {

        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);
        // TODO: We may want to distribute to other event pools if jana:enable_stealing=true

        size_t count = finished_events.size();
        while (count-- > 0 && pool.events.size() < m_pool_size) {
            T* t = finished_events.back();
            finished_events.pop_back();
            release_item(t);
            pool.available_items.push_back(t);
        }
        while (count-- > 0) {
            T* t = finished_events.back();
            finished_events.pop_back();
            release_item(t);
            delete t;
        }
    }
};

