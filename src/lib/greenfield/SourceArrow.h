//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_SOURCEARROW_H
#define GREENFIELD_SOURCEARROW_H

#include <greenfield/Arrow.h>
#include <greenfield/Components.h>

namespace greenfield {

/// SourceArrow wraps a reference to a Source and 'lifts' it into a Arrow
/// that knows how to stream to and from queues, and is executable by a Worker
template<typename T>
class SourceArrow : public Arrow {

private:
    Source<T> & _source;
    Queue<T> * _output_queue;
    std::vector<T> _chunk_buffer;
    bool _is_initialized = false;


public:
    SourceArrow(std::string name, Source<T>& source, Queue<T> *output_queue)
        : Arrow(name, false)
        , _source(source)
        , _output_queue(output_queue) {

        _output_queue->attach_upstream(this);
        attach_downstream(_output_queue);
    }

    StreamStatus execute() {
        if (!is_active()) {
            return StreamStatus::Finished;
        }
        if (!_is_initialized) {
            _source.initialize();
            _is_initialized = true;
        }

        auto start_time = std::chrono::steady_clock::now();
        SourceStatus in_status = _source.inprocess(_chunk_buffer, get_chunksize());
        auto latency_time = std::chrono::steady_clock::now();
        StreamStatus out_status = _output_queue->push(std::move(_chunk_buffer));
        auto message_count = _chunk_buffer.size();
        _chunk_buffer.clear();
        auto finished_time = std::chrono::steady_clock::now();

        auto latency = (latency_time - start_time).count();
        auto overhead = (finished_time - latency_time).count();

        update_message_count(message_count);
        update_total_latency(latency);
        update_total_overhead(overhead);
        set_last_latency(latency/message_count);
        set_last_overhead(overhead/message_count);

        //auto metrics_time = std::chrono::steady_clock::now();
        // TODO: This is only queue overhead. Measure metrics, scheduler overhead later.

        if (in_status == SourceStatus::Finished) {
            _source.finalize();
            set_active(false);
            notify_downstream(false);
            // TODO: We may need to have scheduler check that _all_ threads running
            // this arrow have finished before notifying downstream, otherwise
            // a couple of straggler events ~might~ get stranded on an inactive queue
            return StreamStatus::Finished;
        }
        if (in_status == SourceStatus::KeepGoing && out_status == StreamStatus::KeepGoing) {
            return StreamStatus::KeepGoing;
        }
        return StreamStatus::ComeBackLater;
    }
};

} // namespace greenfield

#endif //GREENFIELD_SOURCEARROW_H
