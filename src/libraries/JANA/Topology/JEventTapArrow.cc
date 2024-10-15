// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/Topology/JEventTapArrow.h>
#include <JANA/Utils/JEventPool.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEvent.h>


JEventTapArrow::JEventTapArrow(std::string name)
        : JPipelineArrow(std::move(name), false, false, false) {}

void JEventTapArrow::add_processor(JEventProcessor* proc) {
    m_procs.push_back(proc);
}

void JEventTapArrow::process(Event* event, bool& success, JArrowMetrics::Status& status) {

    LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << " for event# " << (*event)->GetEventNumber() << LOG_END;
    for (JEventProcessor* proc : m_procs) {
        JCallGraphEntryMaker cg_entry(*(*event)->GetJCallGraphRecorder(), proc->GetTypeName()); // times execution until this goes out of scope
        if (proc->GetCallbackStyle() != JEventProcessor::CallbackStyle::LegacyMode) {
            proc->DoTap(*event);
        }
    }
    LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " for event# " << (*event)->GetEventNumber() << LOG_END;
    success = true;
    status = JArrowMetrics::Status::KeepGoing;
}

void JEventTapArrow::initialize() {
    LOG_DEBUG(m_logger) << "Initializing arrow '" << get_name() << "'" << LOG_END;
    for (auto processor : m_procs) {
        processor->DoInitialize();
        LOG_INFO(m_logger) << "Initialized JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
    }
}

void JEventTapArrow::finalize() {
    LOG_DEBUG(m_logger) << "Finalizing arrow '" << get_name() << "'" << LOG_END;
    for (auto processor : m_procs) {
        processor->DoFinalize();
        LOG_INFO(m_logger) << "Finalized JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
    }
}

