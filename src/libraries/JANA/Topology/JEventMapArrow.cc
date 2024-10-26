// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JANA/JEventProcessor.h"
#include <JANA/Topology/JEventMapArrow.h>

#include <JANA/JEventSource.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEvent.h>


JEventMapArrow::JEventMapArrow(std::string name) {
    set_name(name);
    set_is_parallel(true);
}

void JEventMapArrow::add_source(JEventSource* source) {
    m_sources.push_back(source);
}

void JEventMapArrow::add_unfolder(JEventUnfolder* unfolder) {
    m_unfolders.push_back(unfolder);
}

void JEventMapArrow::add_processor(JEventProcessor* processor) {
    m_procs.push_back(processor);
}

void JEventMapArrow::fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) {

    LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << " for event# " << event->GetEventNumber() << LOG_END;
    for (JEventSource* source : m_sources) {
        JCallGraphEntryMaker cg_entry(*event->GetJCallGraphRecorder(), source->GetTypeName()); // times execution until this goes out of scope
        source->Preprocess(*event);
    }
    for (JEventUnfolder* unfolder : m_unfolders) {
        JCallGraphEntryMaker cg_entry(*event->GetJCallGraphRecorder(), unfolder->GetTypeName()); // times execution until this goes out of scope
        unfolder->Preprocess(*event);
    }
    for (JEventProcessor* processor : m_procs) {
        JCallGraphEntryMaker cg_entry(*event->GetJCallGraphRecorder(), processor->GetTypeName()); // times execution until this goes out of scope
        if (processor->GetCallbackStyle() == JEventProcessor::CallbackStyle::LegacyMode) {
            processor->DoLegacyProcess(event->shared_from_this());
        }
        else {
            processor->DoMap(*event);
        }
    }
    LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " for event# " << event->GetEventNumber() << LOG_END;
    outputs[0] = {event, 1};
    output_count = 1;
    status = JArrowMetrics::Status::KeepGoing;
}

void JEventMapArrow::initialize() {
    LOG_DEBUG(m_logger) << "Initializing arrow '" << get_name() << "'" << LOG_END;
    for (auto processor : m_procs) {
        if (processor->GetCallbackStyle() == JEventProcessor::CallbackStyle::LegacyMode) {
            LOG_INFO(m_logger) << "Initializing JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
            processor->DoInitialize();
            LOG_INFO(m_logger) << "Initialized JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
        }
    }
    LOG_DEBUG(m_logger) << "Initialized arrow '" << get_name() << "'" << LOG_END;
}

void JEventMapArrow::finalize() {
    LOG_DEBUG(m_logger) << "Finalizing arrow '" << get_name() << "'" << LOG_END;
    for (auto processor : m_procs) {
        if (processor->GetCallbackStyle() == JEventProcessor::CallbackStyle::LegacyMode) {
            LOG_DEBUG(m_logger) << "Finalizing JEventProcessor " << processor->GetTypeName() << LOG_END;
            processor->DoFinalize();
            LOG_INFO(m_logger) << "Finalized JEventProcessor " << processor->GetTypeName() << LOG_END;
        }
    }
    LOG_DEBUG(m_logger) << "Finalized arrow " << get_name() << LOG_END;
}

