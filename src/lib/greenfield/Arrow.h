//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_ARROW_H
#define GREENFIELD_ARROW_H

#include <vector>

#include <greenfield/Queue.h>


namespace greenfield {


class Arrow : public Activable {

private:
    // Constants
    const std::string _name;           // Used for human understanding
    const size_t _index;               // Used as an array index
    const bool _is_parallel;           // Whether or not it is safe to parallelize

    // Written internally, read externally
    size_t _message_count = 0;    // Total number of messages completed by this arrow
    double _total_latency = 0;   // Total time spent doing actual work (across all cpus)
    double _total_overhead = 0;  // Total time spent pushing and popping from queues
    double _last_latency = 0;    // Most recent latency measurement (from a single cpu)
    double _last_overhead = 0;   // Most recent time spent pushing and popping from queues

    // Written externally
    int _chunksize = 1;          // Number of items to pop off the input queue at once
    int _thread_count = 0;       // Current number of threads assigned to this arrow

    std::mutex _mutex;       // Protects access to arrow properties.
    // TODO: Replace with atomics when the time is right


public:

    // Constants

    bool is_parallel() { return _is_parallel; }

    std::string get_name() { return _name; }

    size_t get_index() { return _index; }


    // Written internally, read externally

    size_t get_message_count() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _message_count;
    }

    double get_total_latency() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _total_latency;
    }

    double get_total_overhead() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _total_overhead;
    }

    double get_last_latency() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _last_latency;
    }

    double get_last_overhead() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _last_overhead;
    }


protected:

    // Written internally, read externally

    void update_message_count(size_t message_count_delta) {
        std::lock_guard<std::mutex> lock(_mutex);
        _message_count += message_count_delta;
    }

    void update_total_latency(double total_latency_delta) {
        std::lock_guard<std::mutex> lock(_mutex);
        _total_latency += total_latency_delta;
    }

    void update_total_overhead(double total_overhead_delta) {
        std::lock_guard<std::mutex> lock(_mutex);
        _total_overhead += total_overhead_delta;
    }

    void set_last_latency(double last_latency) {
        std::lock_guard<std::mutex> lock(_mutex);
        _last_latency = last_latency;
    }

    void set_last_overhead(double last_overhead) {
        std::lock_guard<std::mutex> lock(_mutex);
        _last_overhead = last_overhead;
    }


public:

    // Written externally

    void set_chunksize(int chunksize) {
        std::lock_guard<std::mutex> lock(_mutex);
        _chunksize = chunksize;
    }

    int get_chunksize() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _chunksize;
    }

    void set_thread_count(int thread_count) {
        std::lock_guard<std::mutex> lock(_mutex);
        _thread_count = thread_count;
    }

    int get_thread_count() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _thread_count;
    }


    Arrow(std::string name, size_t index, bool is_parallel) :
            _name(name), _index(index), _is_parallel(is_parallel) {};

    virtual ~Arrow() = default;

    virtual StreamStatus execute() = 0;

};


} // namespace greenfield


#endif // GREENFIELD_ARROW_H
