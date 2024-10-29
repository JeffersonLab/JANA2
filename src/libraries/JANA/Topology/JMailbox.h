
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/JEvent.h>

#include <mutex>
#include <deque>

/// JMailbox is a threadsafe event queue designed for communication between Arrows.
/// It is different from the standard data structure in the following ways:
///   - pushes and pops return a Status enum, handling the problem of .size() always being stale
///   - pops may fail but pushes may not
///   - pushes and pops are chunked, reducing contention and handling the failure case cleanly
///   - the underlying queue may be shared by all threads, NUMA-domain-local, or thread-local
///   - the Arrow doesn't have to know anything about locality.
///
/// To handle memory locality at different granularities, we introduce the concept of a location.
/// Each thread belongs to exactly one location, represented by contiguous unsigned
/// ints starting at 0. While JArrows are wired to one logical JMailbox, JWorkers interact with
/// the physical LocalQueue corresponding to their location. Locations prevent events from crossing 
/// NUMA domains as they get picked up by different JWorker threads.
///
/// \tparam T must be moveable. Usually this is unique_ptr<JEvent>.
///
/// Improvements:
///   1. Pad LocalQueue
///   2. Enable work stealing


class JQueue {
protected:
    size_t m_capacity;
    size_t m_locations_count;
    bool m_enable_work_stealing = false;
    int m_id = 0;
    JLogger m_logger;

public:
    inline size_t get_threshold() { return m_capacity; }
    inline size_t get_locations_count() { return m_locations_count; }
    inline bool is_work_stealing_enabled() { return m_enable_work_stealing; }
    void set_logger(JLogger logger) { m_logger = logger; }
    void set_id(int id) { m_id = id; }


    inline JQueue(size_t threshold, size_t locations_count, bool enable_work_stealing)
        : m_capacity(threshold), m_locations_count(locations_count), m_enable_work_stealing(enable_work_stealing) {}
    virtual ~JQueue() = default;
};

template <typename T>
class JMailbox : public JQueue {

    struct LocalQueue {
        std::mutex mutex;
        std::deque<T> queue;
    };

    // TODO: Copy these params into DLMB for better locality
    std::unique_ptr<LocalQueue[]> m_queues;

public:

    enum class Status {Ready, Congested, Empty, Full};

    friend std::ostream& operator<<(std::ostream& os, const Status& s) {
        switch (s) {
            case Status::Ready:     os << "Ready"; break;
            case Status::Congested: os << "Congested"; break;
            case Status::Empty:     os << "Empty"; break;
            case Status::Full:      os << "Full"; break;
            default:                os << "Unknown"; break;
        }
        return os;
    }


    /// threshold: the (soft) maximum number of items in the queue at any time
    /// locations_count: the number of locations. More locations = better NUMA performance, worse load balancing
    /// enable_work_stealing: allow events to cross locations only when no other work is available. Improves aforementioned load balancing.
    JMailbox(size_t threshold=100, size_t locations_count=1, bool enable_work_stealing=false)
        : JQueue(threshold, locations_count, enable_work_stealing) {

        m_queues = std::unique_ptr<LocalQueue[]>(new LocalQueue[locations_count]);
    }

    virtual ~JMailbox() {
        //delete [] m_queues;
    }

    // We can do this (for now) because we use a deque underneath, so threshold is 'soft'
    inline void set_threshold(size_t threshold) { m_capacity = threshold; }

    /// size() counts the number of items in the queue across all locations
    /// This should be used sparingly because it will mess up a bunch of caches.
    /// Meant to be used by measure_perf()
    size_t size() {
        size_t result = 0;
        for (size_t i = 0; i<m_locations_count; ++i) {
            std::lock_guard<std::mutex> lock(m_queues[i].mutex);
            result += m_queues[i].queue.size();
        }
        return result;
    };

    /// size(location_id) counts the number of items in the queue for a particular location
    /// Meant to be used by Scheduler::next_assignment() and measure_perf(), eventually
    size_t size(size_t location_id) {
        return m_queues[location_id].queue.size();
    }

    /// push(items, location_id) This function will always succeed.
    Status push(std::vector<T>& buffer, size_t location_id = 0) {

        auto& mb = m_queues[location_id];
        std::lock_guard<std::mutex> lock(mb.mutex);
        for (const T& t : buffer) {
             mb.queue.push_back(std::move(t));
        }
        buffer.clear();
        if (mb.queue.size() > m_capacity) {
            return Status::Full;
        }
        return Status::Ready;
    }


    /// pop() will pop up to requested_count items for the desired location_id.
    /// If many threads are contending for the queue, this will fail with Status::Contention,
    /// in which case the caller should probably consult the Scheduler.
    Status pop(std::vector<T>& buffer, size_t requested_count, size_t location_id = 0) {

        auto& mb = m_queues[location_id];
        if (!mb.mutex.try_lock()) {
            return Status::Congested;
        }
        auto nitems = std::min(requested_count, mb.queue.size());
        buffer.reserve(nitems);
        for (size_t i=0; i<nitems; ++i) {
            buffer.push_back(std::move(mb.queue.front()));
            mb.queue.pop_front();
        }
        auto size = mb.queue.size();
        mb.mutex.unlock();
        if (size >= m_capacity) {
            return Status::Full;
        }
        else if (size != 0) {
            return Status::Ready;
        }
        return Status::Empty;
    }


    Status pop(T& item, bool& success, size_t location_id = 0) {

        success = false;
        auto& mb = m_queues[location_id];
        if (!mb.mutex.try_lock()) {
            return Status::Congested;
        }
        size_t nitems = mb.queue.size();
        if (nitems > 1) {
            item = std::move(mb.queue.front());
            mb.queue.pop_front();
            success = true;
            mb.mutex.unlock();
            return Status::Ready;
        }
        else if (nitems == 1) {
            item = std::move(mb.queue.front());
            mb.queue.pop_front();
            success = true;
            mb.mutex.unlock();
            return Status::Empty;
        }
        mb.mutex.unlock();
        return Status::Empty;
    }

    bool try_push(T* buffer, size_t count, size_t location_id = 0) {
        auto& mb = m_queues[location_id];
        std::lock_guard<std::mutex> lock(mb.mutex);
        if (mb.queue.size() + count > m_capacity) return false;
        for (size_t i=0; i<count; ++i) {
             mb.queue.push_back(buffer[i]);
             buffer[i] = nullptr;
        }
        return true;
    }

    void push(T* buffer, size_t count, size_t location_id = 0) {

        auto& mb = m_queues[location_id];
        std::lock_guard<std::mutex> lock(mb.mutex);
        assert(mb.queue.size() + count <= m_capacity);
        for (size_t i=0; i<count; ++i) {
             mb.queue.push_back(buffer[i]);
             buffer[i] = nullptr;
        }
    }

    size_t pop(T* buffer, size_t min_requested_count, size_t max_requested_count, size_t location_id = 0) {

        auto& mb = m_queues[location_id];
        std::lock_guard<std::mutex> lock(mb.mutex);

        if (mb.queue.size() < min_requested_count) return 0;

        auto nitems = std::min(max_requested_count, mb.queue.size());

        for (size_t i=0; i<nitems; ++i) {
            buffer[i] = mb.queue.front();
            mb.queue.pop_front();
        }
        return nitems;
    }

};

template <>
inline void JMailbox<JEvent*>::push(JEvent** buffer, size_t count, size_t location_id) {

    auto& mb = m_queues[location_id];
    std::lock_guard<std::mutex> lock(mb.mutex);
    assert(mb.queue.size() + count <= m_capacity);
    for (size_t i=0; i<count; ++i) {
        LOG_TRACE(m_logger) << "JMailbox: push_and_unreserve(): queue #" << m_id << ", event #" << buffer[i]->GetEventNumber() << LOG_END;
        mb.queue.push_back(buffer[i]);
        buffer[i] = nullptr;
    }
} 

template <>
inline size_t JMailbox<JEvent*>::pop(JEvent** buffer, size_t min_requested_count, size_t max_requested_count, size_t location_id) {

    auto& mb = m_queues[location_id];
    std::lock_guard<std::mutex> lock(mb.mutex);

    if (mb.queue.size() < min_requested_count) return 0;

    auto nitems = std::min(max_requested_count, mb.queue.size());

    for (size_t i=0; i<nitems; ++i) {
        buffer[i] = mb.queue.front();
        LOG_TRACE(m_logger) << "JMailbox: pop(): queue #" << m_id << ", event #" << buffer[i]->GetEventNumber() << LOG_END;
        mb.queue.pop_front();
    }
    return nitems;
}


