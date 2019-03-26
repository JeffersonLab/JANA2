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
    MapArrow(std::string name, size_t index, ParallelProcessor<S,T>& processor,
             Queue<S> *input_queue, Queue<T> *output_queue)

           : Arrow(name, index, true)
           , _processor(processor)
           , _input_queue(input_queue)
           , _output_queue(output_queue) {

        _input_queue->attach_downstream(this);
        _output_queue->attach_upstream(this);
        attach_upstream(_input_queue);
        attach_downstream(_output_queue);
    };

    StreamStatus execute() {

        std::vector<S> xs;
        std::vector<T> ys;
        xs.reserve(get_chunksize());
        ys.reserve(get_chunksize());
        // TODO: These allocations are unnecessary and should be eliminated

        StreamStatus in_status = _input_queue->pop(xs, get_chunksize());

        for (S &x : xs) {
            ys.push_back(_processor.process(x));
        }
        StreamStatus out_status = StreamStatus::Finished;
        if (!ys.empty()) {
            out_status = _output_queue->push(ys);
        }
        if (in_status == StreamStatus::Finished) {
            notify_downstream();
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
