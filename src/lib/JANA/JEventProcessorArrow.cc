//
// Created by nbrei on 4/8/19.
//

#include <JANA/JEventProcessorArrow.h>
#include <JANA/JEventProcessor.h>

JEventProcessorArrow::JEventProcessorArrow(std::string name,
                                           JEventProcessor& processor,
                                           EventQueue *input_queue,
                                           EventQueue *output_queue)
        : JArrow(std::move(name), true)
        , _processor(processor)
        , _input_queue(input_queue)
        , _output_queue(output_queue)
{
    _input_queue->attach_downstream(this);
    _output_queue->attach_upstream(this);
    attach_upstream(_input_queue);
    attach_downstream(_output_queue);
}


JArrow::Status JEventProcessorArrow::execute() {

    auto start_total_time = std::chrono::steady_clock::now();
    std::vector<Event> xs;
    xs.reserve(get_chunksize());

    auto in_status = _input_queue->pop(xs, get_chunksize());

    auto start_latency_time = std::chrono::steady_clock::now();
    for (Event& x : xs) {
        _processor.Process(x);
    }
    auto message_count = xs.size();
    auto end_latency_time = std::chrono::steady_clock::now();

    auto out_status = EventQueue::Status::Ready;
    if (message_count > 0) {
        out_status = _output_queue->push(xs);
    }
    auto end_queue_time = std::chrono::steady_clock::now();


    auto latency = (end_latency_time - start_latency_time);
    auto overhead = (end_queue_time - start_total_time) - latency;
    update_metrics(message_count, 1, latency, overhead);

    if (in_status == EventQueue::Status::Finished) {
        set_upstream_finished(true);
        return JArrow::Status::Finished;
    } else if (in_status == EventQueue::Status::Ready && out_status == EventQueue::Status::Ready) {
        return JArrow::Status::KeepGoing;
    } else {
        return JArrow::Status::ComeBackLater;
    }
}
