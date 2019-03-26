#pragma once

#include <mutex>
#include <vector>
#include <queue>
#include <assert.h>
#include "Activable.h"

namespace greenfield {

enum class StreamStatus {KeepGoing, ComeBackLater, Finished, Error};

inline std::string to_string(StreamStatus h) {
    switch (h) {
        case StreamStatus::KeepGoing:     return "KeepGoing";
        case StreamStatus::ComeBackLater: return "ComeBackLater";
        case StreamStatus::Finished:      return "Finished";
        default:                          return "Error";
    }
}


class QueueBase : public Activable {

protected:
    size_t _threshold = 128;
    uint32_t _id;

public:

    virtual ~QueueBase() = default;

    uint32_t get_id() { return _id; }
    void set_id(uint32_t id) { _id = id; }

    size_t get_threshold() { return _threshold; }
    void set_threshold(size_t threshold) { _threshold = threshold; }

    virtual size_t get_item_count() = 0;
};


template <typename T>
class Queue : public QueueBase {

private:
    std::mutex _mutex;
    std::deque<T> _underlying;

public:
    size_t get_item_count() final { return _underlying.size(); }

    StreamStatus push(const T& t, bool autoactivate = true) {
        if (autoactivate && !is_active()) {
            set_active(true);
            notify_downstream();
        }
        _mutex.lock();
        _underlying.push_back(t);
        size_t size = _underlying.size();
        _mutex.unlock();

        if (size > _threshold) {
            return StreamStatus::ComeBackLater;
        }
        return StreamStatus::KeepGoing;
    }

    StreamStatus push(const std::vector<T>& buffer, bool autoactivate = true) {
        if (autoactivate && !is_active()) {
            set_active(true);
            notify_downstream();
        }
        _mutex.lock();
        for (const T& t : buffer) {
            _underlying.push_back(t);
        }
        size_t item_count = _underlying.size();
        _mutex.unlock();

        if (item_count > _threshold) {
            return StreamStatus::ComeBackLater;
        }
        return StreamStatus::KeepGoing;
    }

    StreamStatus pop(std::vector<T>& buffer, size_t count) {
        buffer.clear();
        _mutex.lock();
        size_t nitems = std::min(count, _underlying.size());
        buffer.reserve(nitems);
        for (int i=0; i<nitems; ++i) {
            buffer.push_back(std::move(_underlying.front()));
            _underlying.pop_front();
        }
        size_t size = _underlying.size();
        _mutex.unlock();

        if (size != 0) {
            return StreamStatus::KeepGoing;
        }
        if (!is_active()) {
            return StreamStatus::Finished;
        }
        return StreamStatus::ComeBackLater;
    }
};


} // namespace greenfield


