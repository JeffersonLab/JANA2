#pragma once

#include <mutex>
#include <vector>
#include <queue>
#include <assert.h>
#include "JActivable.h"


class QueueBase : public JActivable {

protected:
    size_t _threshold = 128;
    std::string _name = "anon_queue";

public:

    enum class Status { Ready, Congested, Empty, Full, Finished };

    virtual ~QueueBase() = default;

    std::string get_name() { return _name; }
    void set_name(std::string name) { _name = name; }

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

    Status push(const T& t) {
        _mutex.lock();
        _underlying.push_back(t);
        size_t size = _underlying.size();
        _mutex.unlock();

        if (size > _threshold) {
            return Status::Full;
        }
        return Status::Ready;
    }

    Status push(const std::vector<T>& buffer) {
        _mutex.lock();
        for (const T& t : buffer) {
            _underlying.push_back(t);
        }
        size_t item_count = _underlying.size();
        _mutex.unlock();

        if (item_count > _threshold) {
            return Status::Full;
        }
        return Status::Ready;
    }

    Status pop(std::vector<T>& buffer, size_t count) {

        buffer.clear();
        if (!_mutex.try_lock()) {
            return Status::Congested;
        }
        size_t nitems = std::min(count, _underlying.size());
        buffer.reserve(nitems);
        for (int i=0; i<nitems; ++i) {
            buffer.push_back(std::move(_underlying.front()));
            _underlying.pop_front();
        }
        size_t size = _underlying.size();
        _mutex.unlock();

        if (size != 0) {
            return Status::Ready;
        }
        if (is_active()) {
            return Status::Empty;
        }
        return Status::Finished;
    }
};




