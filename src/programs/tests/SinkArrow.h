//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_SINKARROW_H
#define GREENFIELD_SINKARROW_H

#include <JANA/JArrow.h>


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
    Queue<T> * _input_queue;
    std::vector<T> _chunk_buffer;
    bool _is_initialized = false;

public:
    SinkArrow(std::string name, Sink<T>& sink, Queue<T>* input_queue)
        : JArrow(name, false, NodeType::Sink)
        , _sink(sink)
        , _input_queue(input_queue) {

        _input_queue->attach_downstream(this);
        attach_upstream(_input_queue);
    };

    void execute(JArrowMetrics& result) override {
        if (!is_active()) {
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
        if (in_status == Queue<T>::Status::Finished) {
            set_upstream_finished(true);
            set_active(false);
            notify_downstream(false);
            _sink.finalize();
            status = JArrowMetrics::Status::Finished;
        }
        else if (in_status == Queue<T>::Status::Empty) {
            status = JArrowMetrics::Status::ComeBackLater;
        }
        else {
            status = JArrowMetrics::Status::KeepGoing;
        }
        result.update(status, message_count, 1, latency, overhead);
    }

    size_t get_pending() final { return _input_queue->get_item_count(); }

    size_t get_threshold() final { return _input_queue->get_threshold(); }

    void set_threshold(size_t threshold) final { _input_queue->set_threshold(threshold); }
};


#endif //GREENFIELD_SINKARROW_H
