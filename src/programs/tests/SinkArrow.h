
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef GREENFIELD_SINKARROW_H
#define GREENFIELD_SINKARROW_H

#include <JANA/Engine/JArrow.h>


/// Sink consumes events of type T, accumulating state along the way. This
/// state is supposed to reside on the Sink itself.
/// The final result of this accumulation is a side-effect which should be
/// safe to retrieve after finalize() is called. (finalize() will be called
/// automatically after all upstream events have been processed)
/// This is conceptually equivalent to the part of JEventProcessor::Process
/// after the lock is acquired.

template <typename T>
struct Sink {
    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual void outprocess(T t) = 0;
};

/// SinkArrow lifts a sink into a streaming, async context
template<typename T>
class SinkArrow : public JArrow {

private:
    Sink<T> & _sink;
    JMailbox<T> * _input_queue;
    std::vector<T> _chunk_buffer;
    bool _is_initialized = false;

public:
    SinkArrow(std::string name, Sink<T>& sink, JMailbox<T>* input_queue)
        : JArrow(name, false, NodeType::Sink)
        , _sink(sink)
        , _input_queue(input_queue) {
    };

    void execute(JArrowMetrics& result, size_t /* location_id */) override {
        if (get_status() == JActivable::Status::Finished) {
            result.update_finished();
            return;
        }
        if (!_is_initialized) {
            _sink.initialize();
            _chunk_buffer.reserve(get_chunksize());
            _is_initialized = true;
        }

        auto start_time = std::chrono::steady_clock::now();

        auto in_status = _input_queue->pop(_chunk_buffer, get_chunksize());

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

        JArrowMetrics::Status status;
        // TODO: NOT HERE. Finalize the sink in on_status_change().
        // if (in_status == JMailbox<T>::Status::Finished) {
        //     set_status(JActivable::Status::Finished);
        //     _sink.finalize();
        //     status = JArrowMetrics::Status::Finished;
        // }
        if (in_status == JMailbox<T>::Status::Empty) {
            status = JArrowMetrics::Status::ComeBackLater;
        }
        else {
            status = JArrowMetrics::Status::KeepGoing;
        }
        result.update(status, message_count, 1, latency, overhead);
    }

    size_t get_pending() final { return _input_queue->size(); }

    size_t get_threshold() final { return _input_queue->get_threshold(); }

    void set_threshold(size_t threshold) final { _input_queue->set_threshold(threshold); }
};


#endif //GREENFIELD_SINKARROW_H
