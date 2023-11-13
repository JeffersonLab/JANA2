
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/Engine/JEventProcessorArrow.h>
#include <JANA/Utils/JEventPool.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>


JEventProcessorArrow::JEventProcessorArrow(std::string name,
                                           EventQueue *input_queue,
                                           EventQueue *output_queue,
                                           std::shared_ptr<JEventPool> pool)
        : JArrow(std::move(name), true, NodeType::Sink)
        , m_input_queue(input_queue)
        , m_output_queue(output_queue)
        , m_pool(std::move(pool)) {
}

void JEventProcessorArrow::add_processor(JEventProcessor* processor) {
    m_processors.push_back(processor);
}

void JEventProcessorArrow::execute(JArrowMetrics& result, size_t location_id) {

    auto start_total_time = std::chrono::steady_clock::now();

    Event* x;
    bool success;
    auto in_status = m_input_queue->pop(x, success, location_id);
    LOG_TRACE(m_logger) << "JEventProcessorArrow '" << get_name() << "' [" << location_id << "]: "
                        << "pop() returned " << ((success) ? "success" : "failure")
                        << "; queue is now " << in_status << LOG_END;

    auto start_latency_time = std::chrono::steady_clock::now();
    if (success) {
        LOG_DEBUG(m_logger) << "JEventProcessorArrow '" << get_name() << "': Starting event# " << (*x)->GetEventNumber() << LOG_END;
        for (JEventProcessor* processor : m_processors) {
            JCallGraphEntryMaker cg_entry(*(*x)->GetJCallGraphRecorder(), processor->GetTypeName()); // times execution until this goes out of scope
            processor->DoMap(*x);
        }
        LOG_DEBUG(m_logger) << "JEventProcessorArrow '" << get_name() << "': Finished event# " << (*x)->GetEventNumber() << LOG_END;
    }
    auto end_latency_time = std::chrono::steady_clock::now();

    auto out_status = EventQueue::Status::Ready;

    if (success) {
        if (m_output_queue != nullptr) {
            // This is NOT the last arrow in the topology. Pass the event onwards.
            out_status = m_output_queue->push(x, location_id);
        }
        else {
            // This IS the last arrow in the topology. Notify the event source and return event to the pool.
            if( auto es = (*x)->GetJEventSource() ) es->DoFinish(**x);
            m_pool->put(x, location_id);
        }
    }
    auto end_queue_time = std::chrono::steady_clock::now();

    JArrowMetrics::Status status;
    if (in_status == EventQueue::Status::Empty) {
        status = JArrowMetrics::Status::ComeBackLater;
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

    LOG_DEBUG(m_logger) << "Initializing arrow '" << get_name() << "'" << LOG_END;
    for (auto processor : m_processors) {
        processor->DoInitialize();
        LOG_INFO(m_logger) << "Initialized JEventProcessor '" << processor->GetType() << "'" << LOG_END;
    }
}

void JEventProcessorArrow::finalize() {
    LOG_DEBUG(m_logger) << "Finalizing arrow '" << get_name() << "'" << LOG_END;
    for (auto processor : m_processors) {
        processor->DoFinalize();
        LOG_INFO(m_logger) << "Finalized JEventProcessor '" << processor->GetType() << "'" << LOG_END;
    }
}

size_t JEventProcessorArrow::get_pending() {
    return m_input_queue->size();
}

size_t JEventProcessorArrow::get_threshold() {
    return m_input_queue->get_threshold();
}

void JEventProcessorArrow::set_threshold(size_t threshold) {
    m_input_queue->set_threshold(threshold);
}

