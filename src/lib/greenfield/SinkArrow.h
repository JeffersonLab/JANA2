//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_SINKARROW_H
#define GREENFIELD_SINKARROW_H

#include <JANA/JArrow.h>
#include "Components.h"


template<typename T>
class SinkArrow : public JArrow {

private:
    Sink<T> & _sink;
    Queue<T> * _input_queue;
    std::vector<T> _chunk_buffer;
    bool _is_initialized = false;

public:
    SinkArrow(std::string name, Sink<T>& sink, Queue<T>* input_queue)
        : JArrow(name, false)
        , _sink(sink)
        , _input_queue(input_queue) {

        _input_queue->attach_downstream(this);
        attach_upstream(_input_queue);
    };

    JArrow::Status execute() {
        if (!is_active()) {
            return JArrow::Status::Finished;
        }
        if (!_is_initialized) {
            _sink.initialize();
            _chunk_buffer.reserve(get_chunksize());
            _is_initialized = true;
        }

        auto start_time = std::chrono::steady_clock::now();

        auto result = _input_queue->pop(_chunk_buffer, get_chunksize());

        auto latency_start_time = std::chrono::steady_clock::now();

        for (T item : _chunk_buffer) {
            _sink.outprocess(item);
        }
        auto latency_stop_time = std::chrono::steady_clock::now();

        auto message_count = _chunk_buffer.size();
        _chunk_buffer.clear();

        auto stop_time = std::chrono::steady_clock::now();

        auto latency = latency_stop_time - latency_start_time;
        auto overhead = (stop_time - start_time) - latency;
        update_metrics(message_count, 1, latency, overhead);

        if (result == Queue<T>::Status::Finished) {
            set_upstream_finished(true);
            set_active(false);
            notify_downstream(false);
            _sink.finalize();
            return JArrow::Status::Finished;
        }
        else if (result == Queue<T>::Status::Empty) {
            return JArrow::Status::ComeBackLater;
        }
        return JArrow::Status::KeepGoing;
    }
};


#endif //GREENFIELD_SINKARROW_H
