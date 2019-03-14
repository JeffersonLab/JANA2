#pragma once

#include <mutex>
#include <vector>
#include <queue>

enum class SchedulerHint {KeepGoing, ComeBackLater, Finished};

template <typename T>
class Queue {

private:
    std::mutex _mutex;
    std::deque<T> _underlying;

    std::atomic<size_t> _threshold {128};
    std::atomic<bool> _is_finished {false};

public:
    bool is_finished() { return _is_finished; }
    size_t get_item_count() { return _underlying.size(); }
    size_t get_threshold() { return _threshold; }
    void set_threshold(size_t threshold) { _threshold = threshold; }
    void set_finished(bool is_finished) { _is_finished = is_finished; }


    SchedulerHint push(T& t) {
        _mutex.lock();
        assert(_is_finished == false);
        _underlying.push_back(t);
    }

    SchedulerHint push(std::vector<T>& buffer) {
        _mutex.lock();
        assert(_is_finished == false);
        for (T& t : buffer) {
            _underlying.push_back(t);
        }
        size_t item_count = _underlying.size();
        _mutex.unlock();

        if (item_count > _threshold) {
            return SchedulerHint::ComeBackLater;
        }
        return SchedulerHint::KeepGoing;
    }

    SchedulerHint pop(std::vector<T>& buffer, size_t count) {
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
            return SchedulerHint::KeepGoing;
        }
        if (_is_finished) {
            return SchedulerHint::Finished;
        }
        return SchedulerHint::ComeBackLater;
    }
};




