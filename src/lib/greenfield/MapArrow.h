//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_MAPARROW_H
#define GREENFIELD_MAPARROW_H

#include "Components.h"
#include <JANA/JArrow.h>


template<typename S, typename T>
class MapArrow : public JArrow {

private:
    ParallelProcessor<S,T>& _processor;
    Queue<S> *_input_queue;
    Queue<T> *_output_queue;

public:
    MapArrow(std::string name, ParallelProcessor<S,T>& processor, Queue<S> *input_queue, Queue<T> *output_queue)
           : JArrow(name, true)
           , _processor(processor)
           , _input_queue(input_queue)
           , _output_queue(output_queue) {

        _input_queue->attach_downstream(this);
        _output_queue->attach_upstream(this);
        attach_upstream(_input_queue);
        attach_downstream(_output_queue);
    };

    JArrow::Status execute() {

        auto start_total_time = std::chrono::steady_clock::now();
        std::vector<S> xs;
        std::vector<T> ys;
        xs.reserve(get_chunksize());
        ys.reserve(get_chunksize());
        // TODO: These allocations are unnecessary and should be eliminated

        auto in_status = _input_queue->pop(xs, get_chunksize());

        auto start_latency_time = std::chrono::steady_clock::now();
        for (S &x : xs) {
            ys.push_back(_processor.process(x));
        }
        auto message_count = xs.size();
        auto end_latency_time = std::chrono::steady_clock::now();

        auto out_status = QueueBase::Status::Ready;
        if (!ys.empty()) {
            out_status = _output_queue->push(ys);
        }
        auto end_queue_time = std::chrono::steady_clock::now();


        auto latency = (end_latency_time - start_latency_time);
        auto overhead = (end_queue_time - start_total_time) - latency;
        update_metrics(message_count, 1, latency, overhead);


        if (in_status == QueueBase::Status::Finished) {
            set_upstream_finished(true);
            return JArrow::Status::Finished;
        }
        else if (in_status == QueueBase::Status::Ready && out_status == QueueBase::Status::Ready) {
            return JArrow::Status::KeepGoing;
        }
        else {
            return JArrow::Status::ComeBackLater;
        }
    }
};


#endif //GREENFIELD_MAPARROW_H
