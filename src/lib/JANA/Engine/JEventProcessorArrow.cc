//
// Created by nbrei on 4/8/19.
//

#include <JANA/Engine/JEventProcessorArrow.h>
#include <JANA/Utils/JEventPool.h>
#include <JANA/JEventProcessor.h>


JEventProcessorArrow::JEventProcessorArrow(std::string name,
                                           EventQueue *input_queue,
                                           EventQueue *output_queue,
                                           std::shared_ptr<JEventPool> pool)
        : JArrow(std::move(name), true, NodeType::Sink)
        , _input_queue(input_queue)
        , _output_queue(output_queue)
        , _pool(std::move(pool)) {

    _input_queue->attach_downstream(this);
    attach_upstream(_input_queue);
    _logger = JLogger();

    if (_output_queue != nullptr) {
        _output_queue->attach_upstream(this);
        attach_downstream(_output_queue);
    }
}

void JEventProcessorArrow::add_processor(JEventProcessor* processor) {
    _processors.push_back(processor);
}

void JEventProcessorArrow::execute(JArrowMetrics& result, size_t location_id) {

    auto start_total_time = std::chrono::steady_clock::now();

    Event x;
    bool success;
    auto in_status = _input_queue->pop(x, success, location_id);
    LOG_DEBUG(_logger) << "EventProcessorArrow '" << get_name() << "' [" << location_id << "]: "
                       << "pop() returned " << ((success) ? "success" : "failure")
                       << "; queue is now " << in_status << LOG_END;

    auto start_latency_time = std::chrono::steady_clock::now();
    if (success) {
        LOG_DEBUG(_logger) << "EventProcessorArrow '" << get_name() << "': Starting event# " << x->GetEventNumber() << LOG_END;
        for (JEventProcessor* processor : _processors) {
            processor->DoMap(x);
        }
        LOG_DEBUG(_logger) << "EventProcessorArrow '" << get_name() << "': Finished event# " << x->GetEventNumber() << LOG_END;
    }
    auto end_latency_time = std::chrono::steady_clock::now();

    auto out_status = EventQueue::Status::Ready;

    if (success) {
        if (_output_queue != nullptr) {
            out_status = _output_queue->push(x, location_id);
        }
        else {
            _pool->put(x, location_id);
        }
    }
    auto end_queue_time = std::chrono::steady_clock::now();

    JArrowMetrics::Status status;
    if (in_status == EventQueue::Status::Finished) {
        set_upstream_finished(true);
        status = JArrowMetrics::Status::Finished;
    }
    else if (in_status == EventQueue::Status::Ready && out_status == EventQueue::Status::Ready) {
        status = JArrowMetrics::Status::KeepGoing;
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
    }
    auto latency = (end_latency_time - start_latency_time);
    auto overhead = (end_queue_time - start_total_time) - latency;
    result.update(status, success, 1, latency, overhead);
}

void JEventProcessorArrow::initialize() {

    for (auto processor : _processors) {
        processor->DoInitialize();
    }
}

void JEventProcessorArrow::finalize() {
    for (auto processor : _processors) {
        processor->DoFinalize();
    }
}

size_t JEventProcessorArrow::get_pending() {
    return _input_queue->size();
}

size_t JEventProcessorArrow::get_threshold() {
    return _input_queue->get_threshold();
}

void JEventProcessorArrow::set_threshold(size_t threshold) {
    _input_queue->set_threshold(threshold);
}

