
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/Topology/JEventProcessorArrow.h>
#include <JANA/Utils/JEventPool.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>


JEventProcessorArrow::JEventProcessorArrow(std::string name,
                                           EventQueue *input_queue,
                                           EventQueue *output_queue,
                                           JEventPool *pool)
        : JPipelineArrow(std::move(name),
                         true,
                         false,
                         true,
                         input_queue,
                         output_queue,
                         pool) {}

void JEventProcessorArrow::add_processor(JEventProcessor* processor) {
    m_processors.push_back(processor);
}

void JEventProcessorArrow::process(Event* event, bool& success, JArrowMetrics::Status& status) {
    

    LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << " for event# " << (*event)->GetEventNumber() << LOG_END;
    for (JEventProcessor* processor : m_processors) {
        // TODO: Move me into JEventProcessor::DoMap
        JCallGraphEntryMaker cg_entry(*(*event)->GetJCallGraphRecorder(), processor->GetTypeName()); // times execution until this goes out of scope
        if (processor->GetCallbackStyle() == JEventProcessor::CallbackStyle::LegacyMode) {
            processor->DoLegacyProcess(*event);
        }
        else {
            processor->DoMap(*event);
            processor->DoTap(*event);

        }
    }
    LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " for event# " << (*event)->GetEventNumber() << LOG_END;
    success = true;
    status = JArrowMetrics::Status::KeepGoing;
}

void JEventProcessorArrow::initialize() {
    LOG_DEBUG(m_logger) << "Initializing arrow '" << get_name() << "'" << LOG_END;
    for (auto processor : m_processors) {
        LOG_INFO(m_logger) << "Initializing JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
        processor->DoInitialize();
        LOG_INFO(m_logger) << "Initialized JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
    }
    LOG_DEBUG(m_logger) << "Initialized arrow '" << get_name() << "'" << LOG_END;
}

void JEventProcessorArrow::finalize() {
    LOG_DEBUG(m_logger) << "Finalizing arrow " << get_name() << LOG_END;
    for (auto processor : m_processors) {
        LOG_DEBUG(m_logger) << "Finalizing JEventProcessor " << processor->GetTypeName() << LOG_END;
        processor->DoFinalize();
        LOG_INFO(m_logger) << "Finalized JEventProcessor " << processor->GetTypeName() << LOG_END;
    }
    LOG_DEBUG(m_logger) << "Finalized arrow " << get_name() << LOG_END;
}

