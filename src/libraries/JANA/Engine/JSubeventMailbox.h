
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JSUBEVENTMAILBOX_H
#define JANA2_JSUBEVENTMAILBOX_H

#include <queue>
#include <mutex>
#include <JANA/Services/JLoggingService.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JCpuInfo.h>

#ifndef CACHE_LINE_BYTES
#define CACHE_LINE_BYTES JCpuInfo::JANA2_CACHE_LINE_BYTES
#endif


template <typename SubeventT>
struct SubeventWrapper {

    std::shared_ptr<JEvent> parent;
    SubeventT* data;
    size_t id;
    size_t total;

    SubeventWrapper(std::shared_ptr<JEvent> parent, SubeventT* data, size_t id, size_t total)
    : parent(std::move(parent))
    , data(data)
    , id(id)
    , total(total) {}
};


template <typename SubeventT>
class JSubeventMailbox {

private:

    struct alignas(CACHE_LINE_BYTES) LocalMailbox {
        std::mutex mutex;
        std::deque<std::shared_ptr<JEvent>> ready;
        std::map<std::shared_ptr<JEvent>, size_t> in_progress;
        size_t reserved_count = 0;
    };

    size_t m_threshold;
    size_t m_locations_count;
    bool m_enable_work_stealing = false;
    std::unique_ptr<LocalMailbox[]> m_mailboxes;
    JLogger m_logger;

public:

    enum class Status {Ready, Congested, Empty, Full, Finished};

    friend std::ostream& operator<<(std::ostream& os, const Status& s) {
        switch (s) {
            case Status::Ready:     os << "Ready"; break;
            case Status::Congested: os << "Congested"; break;
            case Status::Empty:     os << "Empty"; break;
            case Status::Full:      os << "Full"; break;
            case Status::Finished:  os << "Finished"; break;
            default:                os << "Unknown"; break;
        }
        return os;
    }


    /// threshold: the (soft) maximum number of items in the queue at any time
    /// domain_count: the number of domains
    /// enable_work_stealing: allow domains to pop from other domains' queues when theirs is empty
    JSubeventMailbox(size_t threshold=100, size_t locations_count=1, bool enable_work_stealing=false)
        : m_threshold(threshold)
        , m_locations_count(locations_count)
        , m_enable_work_stealing(enable_work_stealing) {

        m_mailboxes = std::unique_ptr<LocalMailbox[]>(new LocalMailbox[locations_count]);
    }

    virtual ~JSubeventMailbox() {
        //delete [] m_mailboxes;
    }

    /// size() counts the number of items in the queue across all domains
    /// This should be used sparingly because it will mess up a bunch of caches.
    /// Meant to be used by measure_perf()
    size_t size() {
        size_t result = 0;
        for (size_t i = 0; i<m_locations_count; ++i) {
            std::lock_guard<std::mutex> lock(m_mailboxes[i].mutex);
            result += m_mailboxes[i].queue.size();
        }
        return result;
    };

    /// size(domain) counts the number of items in the queue for a particular domain
    /// Meant to be used by Scheduler::next_assignment() and measure_perf(), eventually
    size_t size(size_t domain) {
        return m_mailboxes[domain].queue.size();
    }

    /// reserve(requested_count) keeps our queues bounded in size. The caller should
    /// reserve their desired chunk size on the output queue first. The output
    /// queue will return a reservation which is less than or equal to requested_count.
    /// The caller may then request as many items from the input queue as have been
    /// reserved on the output queue. Note that because the input queue may return
    /// fewer items than requested, the caller must push their original reserved_count
    /// alongside the items, to avoid a "reservation leak".
    size_t reserve(size_t requested_count, size_t domain = 0) {

        LocalMailbox& mb = m_mailboxes[domain];
        std::lock_guard<std::mutex> lock(mb.mutex);
        size_t doable_count = m_threshold - mb.queue.size() - mb.reserved_count;
        if (doable_count > 0) {
            size_t reservation = std::min(doable_count, requested_count);
            mb.reserved_count += reservation;
            return reservation;
        }
        return 0;
    };

    /// push(items, reserved_count, domain) This function will always
    /// succeed, although it may exceed the threshold if the caller didn't reserve
    /// space, and it may take a long time because it will wait on a mutex.
    /// Note that if the caller had called reserve(), they must pass in the reserved_count here.
    Status push(std::vector<SubeventWrapper<SubeventT>>& buffer, size_t reserved_count = 0, size_t domain = 0) {

        auto& mb = m_mailboxes[domain];
        std::lock_guard<std::mutex> lock(mb.mutex);
        mb.reserved_count -= reserved_count;
        for (const auto& subevent : buffer) {

            // Problem: Are we sure we are updating the event in a way which is effectively thread-safe?
            // Should we be doing this insert, or should the caller?
            subevent.parent->template Insert<SubeventT>(subevent.data);
            if (subevent.total == 1) {
                // Goes straight into "ready"
                mb.ready.push_back(subevent.parent);
            }
            else {
                auto pair = mb.in_progress.find(subevent.parent);
                if (pair == mb.in_progress.end()) {
                    mb.in_progress[subevent.parent] = subevent.total-1;
                }
                else {
                    if (pair->second == 1) {
                        mb.ready.push_back(subevent.parent);
                    }
                    else {
                        pair->second -= 1;
                    }
                }
            }
        }
        buffer.clear();
        // if (mb.in_progress.size() > m_threshold) {
        //     return Status::Full;
        // }
        return Status::Ready;
    }


    /// pop() will pop up to requested_count items for the desired domain.
    /// If many threads are contending for the queue, this will fail with Status::Contention,
    /// in which case the caller should probably consult the Scheduler.
    Status pop(std::vector<std::shared_ptr<JEvent>>& buffer, size_t requested_count, size_t location_id = 0) {

        auto& mb = m_mailboxes[location_id];
        if (!mb.mutex.try_lock()) {
            return Status::Congested;
        }
        auto nitems = std::min(requested_count, mb.ready.size());
        buffer.reserve(nitems);
        for (size_t i=0; i<nitems; ++i) {
            buffer.push_back(std::move(mb.ready.front()));
            mb.ready.pop_front();
        }
        auto size = mb.ready.size();
        mb.mutex.unlock();
        if (size >= m_threshold) {
            return Status::Full;
        }
        else if (size != 0) {
            return Status::Ready;
        }
        return Status::Empty;
    }

#if 0
    Status pop(std::shared_ptr<JEvent> item, bool& success, size_t location_id = 0) {

        success = false;
        auto& mb = m_mailboxes[location_id];
        if (!mb.mutex.try_lock()) {
            return Status::Congested;
        }
        size_t nitems = mb.ready.size();
        if (nitems > 1) {
            item = std::move(mb.ready.front());
            mb.ready.pop_front();
            success = true;
            mb.mutex.unlock();
            return Status::Ready;
        }
        else if (nitems == 1) {
            item = std::move(mb.ready.front());
            mb.ready.pop_front();
            success = true;
            mb.mutex.unlock();
            return Status::Empty;
        }
        else if (is_active()) {
            mb.mutex.unlock();
            return Status::Empty;
        }
        mb.mutex.unlock();
        return Status::Finished;
    }

#endif

    size_t get_threshold() { return m_threshold; }
    void set_threshold(size_t threshold) { m_threshold = threshold; }

};



#endif //JANA2_JSUBEVENTMAILBOX_H
