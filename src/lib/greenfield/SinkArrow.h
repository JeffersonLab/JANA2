//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_SINKARROW_H
#define GREENFIELD_SINKARROW_H

#include <greenfield/Arrow.h>
#include <greenfield/Components.h>

namespace greenfield {

template<typename T>
class SinkArrow : public Arrow {

private:
    Sink<T> & _sink;
    Queue<T> * _input_queue;
    std::vector<T> _chunk_buffer;
    bool _is_initialized = false;

public:
    SinkArrow(std::string name, Sink<T>& sink, Queue<T>* input_queue)
        : Arrow(name, false)
        , _sink(sink)
        , _input_queue(input_queue) {

        _input_queue->attach_downstream(this);
        attach_upstream(_input_queue);
    };

    StreamStatus execute() {
        if (!is_active()) {
            return StreamStatus::Finished;
        }
        if (!_is_initialized) {
            _sink.initialize();
            _chunk_buffer.reserve(get_chunksize());
            _is_initialized = true;
        }
        auto start_time = std::chrono::steady_clock::now();

        StreamStatus result = _input_queue->pop(_chunk_buffer, get_chunksize());

        auto latency_start_time = std::chrono::steady_clock::now();

        for (T item : _chunk_buffer) {
            _sink.outprocess(item);
        }
        auto latency_stop_time = std::chrono::steady_clock::now();

        auto message_count = _chunk_buffer.size();
        _chunk_buffer.clear();

        auto stop_time = std::chrono::steady_clock::now();

        auto latency = (latency_stop_time - latency_start_time).count();
        auto overhead = (stop_time - start_time).count() - latency;

        update_total_overhead(overhead);

        if (message_count != 0) {
            update_message_count(message_count);
            update_total_latency(latency);
            set_last_latency(latency/message_count);
            set_last_overhead(overhead/message_count);
        }

        if (result == StreamStatus::Finished) {
            set_active(false);
            notify_downstream(false);
            _sink.finalize();
        }
        return result;
    }
};

} // namespace greenfield

#endif //GREENFIELD_SINKARROW_H
