
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef GREENFIELD_MAPARROW_H
#define GREENFIELD_MAPARROW_H

#include <JANA/Engine/JArrow.h>


/// ParallelProcessor transforms S to T and it does so in a way which is thread-safe
/// and ideally stateless. It is conceptually equivalent to the first part
/// of JEventProcessor::Process, i.e. up until the lock is acquired. Alternatively, it could
/// become a JFactorySet, in which case process() would call all Factories present, thereby
/// making sure that everything which can be calculated in parallel has in fact been, before
/// proceeding to the (sequential) Sink.

template <typename S, typename T>
struct ParallelProcessor {
    virtual T process(S s) = 0;
};


/// MapArrow lifts a ParallelProcessor into a streaming async context
template<typename S, typename T>
class MapArrow : public JArrow {

private:
    ParallelProcessor<S,T>& _processor;
    JMailbox<S> *_input_queue;
    JMailbox<T> *_output_queue;

public:
    MapArrow(std::string name, ParallelProcessor<S,T>& processor, JMailbox<S> *input_queue, JMailbox<T> *output_queue)
           : JArrow(name, true, false, false)
           , _processor(processor)
           , _input_queue(input_queue)
           , _output_queue(output_queue) {
    };

    void execute(JArrowMetrics& result, size_t /* location_id */) override {

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

        auto out_status = JMailbox<T>::Status::Ready;
        if (!ys.empty()) {
            out_status = _output_queue->push(ys);
        }
        auto end_queue_time = std::chrono::steady_clock::now();


        auto latency = (end_latency_time - start_latency_time);
        auto overhead = (end_queue_time - start_total_time) - latency;

        JArrowMetrics::Status status;
        // if (in_status == JMailbox<S>::Status::Finished) {
        //     set_status(JActivable::Status::Finished);
        //     status = JArrowMetrics::Status::Finished;
        // }
        if (in_status == JMailbox<S>::Status::Ready && out_status == JMailbox<T>::Status::Ready) {
            status = JArrowMetrics::Status::KeepGoing;
        }
        else {
            status = JArrowMetrics::Status::ComeBackLater;
        }
        result.update(status, message_count, 1, latency, overhead);
    }

    size_t get_pending() final { return _input_queue->size(); }

    size_t get_threshold() final { return _input_queue->get_threshold(); }

    void set_threshold(size_t threshold) final { _input_queue->set_threshold(threshold); }
};


#endif //GREENFIELD_MAPARROW_H
