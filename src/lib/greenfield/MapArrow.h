//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_MAPARROW_H
#define GREENFIELD_MAPARROW_H

#include <greenfield/Components.h>
#include <greenfield/Arrow.h>

namespace greenfield {

template<typename S, typename T>
class MapArrow : public Arrow {

private:
    ParallelProcessor<S,T>& _processor;
    Queue<S> *_input_queue;
    Queue<T> *_output_queue;

public:
    MapArrow(std::string name, ParallelProcessor<S,T>& processor, Queue<S> *input_queue, Queue<T> *output_queue)
           : Arrow(name, true)
           , _processor(processor)
           , _input_queue(input_queue)
           , _output_queue(output_queue) {

        _input_queue->attach_downstream(this);
        _output_queue->attach_upstream(this);
        attach_upstream(_input_queue);
        attach_downstream(_output_queue);
    };

    StreamStatus execute() {

        auto start_total_time = std::chrono::steady_clock::now();
        std::vector<S> xs;
        std::vector<T> ys;
        xs.reserve(get_chunksize());
        ys.reserve(get_chunksize());
        // TODO: These allocations are unnecessary and should be eliminated

        StreamStatus in_status = _input_queue->pop(xs, get_chunksize());

        auto start_latency_time = std::chrono::steady_clock::now();
        for (S &x : xs) {
            ys.push_back(_processor.process(x));
        }
        auto message_count = xs.size();
        auto end_latency_time = std::chrono::steady_clock::now();

        StreamStatus out_status = StreamStatus::Finished;
        if (!ys.empty()) {
            out_status = _output_queue->push(ys);
        }
        auto end_queue_time = std::chrono::steady_clock::now();


        auto latency = (end_latency_time - start_latency_time).count();
        auto overhead = (end_queue_time - start_total_time).count() - latency;

        update_queue_visits(1);
        update_total_overhead(overhead);

        if (message_count != 0) {

            update_message_count(message_count);
            update_total_latency(latency);
            set_last_latency(latency/message_count);
            set_last_overhead(overhead/message_count);
        }

        // TODO: Measure metrics overhead
        //auto end_metrics_time = std::chrono::steady_clock::now();

        if (in_status == StreamStatus::Finished) {
            set_active(false);
            notify_downstream(false);
            return StreamStatus::Finished;
        }
        else if (in_status == StreamStatus::KeepGoing && out_status == StreamStatus::KeepGoing) {
            return StreamStatus::KeepGoing;
        }
        else {
            return StreamStatus::ComeBackLater;
        }
    }
};

} // namespace greenfield

#endif //GREENFIELD_MAPARROW_H
