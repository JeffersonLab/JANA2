
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
        : JPipelineArrow(std::move(name),
                         true,
                         NodeType::Sink,
                         input_queue,
                         output_queue,
                         pool.get()) {}

void JEventProcessorArrow::add_processor(JEventProcessor* processor) {
    m_processors.push_back(processor);
}

JArrowMetrics::Status JEventProcessorArrow::process(Event* event) {
    LOG_DEBUG(m_logger) << "JEventProcessorArrow '" << get_name() << "': Starting event# " << (*event)->GetEventNumber() << LOG_END;
    for (JEventProcessor* processor : m_processors) {
        // TODO: Move me into JEventProcessor::DoMap
        JCallGraphEntryMaker cg_entry(*(*event)->GetJCallGraphRecorder(), processor->GetTypeName()); // times execution until this goes out of scope
        processor->DoMap(*event);
    }
    LOG_DEBUG(m_logger) << "JEventProcessorArrow '" << get_name() << "': Finished event# " << (*event)->GetEventNumber() << LOG_END;
    return JArrowMetrics::Status::KeepGoing;
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

