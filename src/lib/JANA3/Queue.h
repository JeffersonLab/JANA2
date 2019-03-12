#pragma once

#include <mutex>
#include <vector>

template <typename T>
class Queue {

  size_t empty_threshold = 16;
  size_t full_threshold = 128;
  bool is_finished = false;

  size_t _item_count;
  T* _buffer;
  std::mutex _mutex;

 public:
  Queue(size_t buffer_size, size_t refill_threshold)
    : _buffer_size(buffer_size), _refill_threshold(refill_threshold) {

    _buffer = new T[buffer_size];
  }

  ~Queue() {
    delete[] _buffer;
  }

  void push(vector<T> submissions);
  // We could also have this return a size_t of _buffer_size - _item_count
  // Or we could have it return a bool indicating "keep going" or not.
  // However, this info gets stale and we want to make decisions about thread assignments
  // at a higher level


  vector<T> pop(size_t chunksize);


  // Idea: Queue size >> nthreads: Each worker only needs to check that there is enough
  // room in the queue for all threads to add one more chunk. If not, then he doesn't take another
  // chunk from . Problem: Variable chunk output

  // Idea: If push() fails, thread yields and wakes consumer
  //       If pop() fails, thread yields and wakes producer

  // Idea: If push() fails, throw away work and not report work as being complete

  // Idea: Make queue growable, have two thresholds for activating and deactivating producer

  // TODO: Think about interface for put and get. Use iterators? Use move semantics?

  // TODO: Think about T. Do we want a custom Task, or a std::packaged_task, or a simple Event?

  // TODO: getProducerTask() ? How does the ThreadManager know how to refill the queue?

}



