//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_SOURCEARROW_H
#define GREENFIELD_SOURCEARROW_H

#include <JANA/JArrow.h>
#include "Components.h"

/// SourceArrow wraps a reference to a Source and 'lifts' it into a Arrow
/// that knows how to stream to and from queues, and is executable by a Worker
template<typename T>
class SourceArrow : public JArrow {

private:
    Source<T> & _source;
    Queue<T> * _output_queue;
    std::vector<T> _chunk_buffer;
    bool _is_initialized = false;


public:
    SourceArrow(std::string name, Source<T>& source, Queue<T> *output_queue)
        : JArrow(name, false)
        , _source(source)
        , _output_queue(output_queue) {

        _output_queue->attach_upstream(this);
        attach_downstream(_output_queue);
    }

    JArrow::Status execute() {
        if (!is_active()) {
            return JArrow::Status::Finished;
        }
        if (!_is_initialized) {
            _source.initialize();
            _is_initialized = true;
        }

        auto start_time = std::chrono::steady_clock::now();
        auto in_status = _source.inprocess(_chunk_buffer, get_chunksize());
        auto latency_time = std::chrono::steady_clock::now();
        auto out_status = _output_queue->push(std::move(_chunk_buffer));
        auto message_count = _chunk_buffer.size();
        _chunk_buffer.clear();
        auto finished_time = std::chrono::steady_clock::now();

        auto latency = (latency_time - start_time);
        auto overhead = (finished_time - latency_time);
        update_metrics(message_count, 1, latency, overhead);


        if (in_status == Source<T>::Status::Finished) {
            _source.finalize();
            set_upstream_finished(true);
            set_active(false);
            notify_downstream(false);
            // TODO: We may need to have scheduler check that _all_ threads running
            // this arrow have finished before notifying downstream, otherwise
            // a couple of straggler events ~might~ get stranded on an inactive queue
            return JArrow::Status::Finished;
        }
        if (in_status == Source<T>::Status::KeepGoing && out_status == QueueBase::Status::Ready) {
            return JArrow::Status::KeepGoing;
        }
        return JArrow::Status::ComeBackLater;
    }
};


#endif //GREENFIELD_SOURCEARROW_H
