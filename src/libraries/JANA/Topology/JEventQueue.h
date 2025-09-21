
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: Nathan Brei

#pragma once
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/JEvent.h>

#include <memory>
#include <vector>
#include <cassert>

// JEventQueue is a queue designed for communication between JArrows, with the following features:
//
// - Fixed-capacity during processing, since the max number of events in flight is predetermined and small
// - Ringbuffer-based, to avoid allocations during processing
// - Adjustable-capacity outside processing, since the max number of events in flight may change when the threadcount does
// - Locality-aware, so that events that live in one location (e.g. NUMA domain) can stay within that location
// - NOT thread-safe, because all queue accesses are protected by the JExecutionEngine mutex.
//
// - Previous versions of JEventQueue have allowed work stealing (taking events out of a different 
//   NUMA domain when none are otherwise available). The current version does not, though we may
//   bring this feature back later.
//
/// To handle memory locality at different granularities, we introduce the concept of a location.
/// Each thread belongs to exactly one location, represented by contiguous unsigned
/// ints starting at 0. While JArrows are wired to one logical JEventQueue, threads interact with
/// the physical LocalQueue corresponding to their location. Locations prevent events from crossing 
/// NUMA domains as they get picked up by different worker threads.

class JEventQueue {

protected:
    struct alignas(JANA2_CACHE_LINE_BYTES) LocalQueue {
        std::vector<JEvent*> ringbuffer;
        size_t size=0;
        size_t capacity=0;
        size_t front=0;
        size_t back=0;
    };

    std::vector<std::unique_ptr<LocalQueue>> m_local_queues;
    size_t m_capacity = 0;
    bool m_enforces_ordering = false;

public:
    inline JEventQueue(size_t initial_capacity, size_t locations_count) {

        assert(locations_count >= 1);
        for (size_t location=0; location<locations_count; ++location) {
            m_local_queues.push_back(std::make_unique<LocalQueue>());
        }
        Scale(initial_capacity);
    }

    virtual ~JEventQueue() = default;

    void EnableOrdering() {
        m_enforces_ordering = true;
        if (m_local_queues.size() != 1) {
            m_local_queues.clear();
            m_local_queues.push_back(std::make_unique<LocalQueue>());
            Scale(m_capacity);
        }
    }

    virtual void Scale(size_t capacity) {
        if (capacity < m_capacity) {
            for (auto& local_queue : m_local_queues) {
                if (local_queue->size != 0) {
                    throw JException("Attempted to shrink a non-empty JEventQueue. Please drain the topology before attempting to downscale.");
                }
            }
        }
        m_capacity = capacity;
        for (auto& local_queue: m_local_queues) {
            local_queue->ringbuffer.resize(capacity, nullptr);
            local_queue->capacity = capacity;
        }
    }

    inline size_t GetLocationCount() {
        return m_local_queues.size();
    }

    inline size_t GetSize(size_t location) {
        auto& local_queue = m_local_queues[location];
        return local_queue->size;
    }

    inline size_t GetCapacity() {
        return m_capacity;
    }

    inline void Push(JEvent* event, size_t location) {
        auto& local_queue = *m_local_queues[location];
        if (local_queue.size == local_queue.capacity) {
            throw JException("Attempted to push to a full JEventQueue. This probably means there is an error in your topology wiring");
        }

        local_queue.ringbuffer[local_queue.front] = event;
        local_queue.front = (local_queue.front + 1) % local_queue.capacity;
        local_queue.size += 1;
    }

    inline JEvent* Pop(size_t location) {
        auto& local_queue= *m_local_queues[location];
        if (local_queue.size == 0) {
            return nullptr;
        }
        JEvent* result = local_queue.ringbuffer[local_queue.back];
        local_queue.ringbuffer[local_queue.back] = nullptr;
        local_queue.back = (local_queue.back + 1) % local_queue.capacity;
        local_queue.size -= 1;
        return result;
    };

};


