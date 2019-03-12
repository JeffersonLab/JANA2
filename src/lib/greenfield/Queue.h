#pragma once

#include <mutex>
#include <vector>
#include <queue>

template <typename T>
class Queue {

    std::mutex _mutex;
    std::queue<T> _underlying;

public:
    const size_t empty_threshold = 16;
    const size_t full_threshold = 128;
    std::atomic<size_t> item_count {0};
    std::atomic<bool> is_finished {false};


    void push(std::vector<T> items) {
        _underlying.push();
    }
    // We could also have this return a size_t of _buffer_size - _item_count
    // Or we could have it return a bool indicating "keep going" or not.
    // However, this info gets stale and we want to make decisions about thread assignments
    // at a higher level


    std::vector<T> pop(size_t item_count) {
        _underlying.top();
        _underlying.pop();
    };


    // Idea: Queue size >> nthreads: Each worker only needs to check that there is enough
    // room in the queue for all threads to add one more chunk. If not, then he doesn't take another
    // chunk from . Problem: Variable chunk output

    // TODO: Think about interface for put and get. Use iterators? Use move semantics?

    // TODO: Think about T. Do we want a custom Task, or a std::packaged_task, or a simple Event?

    // TODO: getProducerTask() ? How does the ThreadManager know how to refill the queue?

};



