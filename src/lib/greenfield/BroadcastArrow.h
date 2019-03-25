//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_BROADCASTARROW_H
#define GREENFIELD_BROADCASTARROW_H

#include <greenfield/Arrow.h>

namespace greenfield {

template<typename T>
class BroadcastArrow : public Arrow {

private:
    Queue<T> *_input_queue;

public:
    BroadcastArrow(std::string name, size_t index, Queue<T> *input_queue, std::vector<Queue<T> *> output_queues)
        : Arrow(name, index, true)
        , _input_queue(input_queue)
    {
        _output_queues = output_queues;
    }


    StreamStatus execute() {

        std::vector<T> items; // TODO: Get rid of this allocation
        items.reserve(get_chunksize());

        StreamStatus in_status = _input_queue->pop(items, get_chunksize());
        bool output_status_keepgoing = true;

        for (Queue<T> *queue : _output_queues) {
            StreamStatus out_status = queue->push(items);
            output_status_keepgoing &= (out_status == StreamStatus::KeepGoing);
        }
        if (in_status == StreamStatus::Finished) {
            return StreamStatus::Finished;
        }
        else if (in_status == StreamStatus::KeepGoing && output_status_keepgoing) {
            return StreamStatus::KeepGoing;
        }
        else {
            return StreamStatus::ComeBackLater;
        }
    }

};

} // namespace greenfield

#endif //GREENFIELD_BROADCASTARROW_H
