// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/Topology/JEventTapArrow.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEvent.h>


JEventTapArrow::JEventTapArrow(std::string name, JEventLevel level) {
    SetName(name);
    AddPort("in", level);
    AddPort("out", level);
}

void JEventTapArrow::add_processor(JEventProcessor* proc) {
    if (proc->IsOrderingEnabled()) {
        m_ports[EVENT_IN]->SetEnforcesOrdering(true);
    }
    m_procs.push_back(proc);
}

void JEventTapArrow::Fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) {

    LOG_DEBUG(m_logger) << "Executing arrow " << GetName() << " for event# " << event->GetEventNumber() << LOG_END;
    for (JEventProcessor* proc : m_procs) {
        JCallGraphEntryMaker cg_entry(*event->GetJCallGraphRecorder(), proc->GetTypeName()); // times execution until this goes out of scope
        if (proc->GetCallbackStyle() != JEventProcessor::CallbackStyle::LegacyMode) {
            proc->DoTap(*event);
        }
    }
    outputs[0] = {event, 1};
    output_count = 1;
    status = JArrow::FireResult::KeepGoing;
    LOG_DEBUG(m_logger) << "Executed arrow " << GetName() << " for event# " << event->GetEventNumber() << LOG_END;
}

void JEventTapArrow::Initialize() {
    LOG_DEBUG(m_logger) << "Initializing arrow '" << GetName() << "'" << LOG_END;
    for (auto processor : m_procs) {
        LOG_DEBUG(m_logger) << "Initializing JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
        processor->DoInit();
        LOG_INFO(m_logger) << "Initialized JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
    }
    LOG_DEBUG(m_logger) << "Initialized arrow '" << GetName() << "'" << LOG_END;
}

void JEventTapArrow::Finalize() {
    LOG_DEBUG(m_logger) << "Finalizing arrow '" << GetName() << "'" << LOG_END;
    for (auto processor : m_procs) {
        LOG_DEBUG(m_logger) << "Finalizing JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
        processor->DoFinalize();
        LOG_INFO(m_logger) << "Finalized JEventProcessor '" << processor->GetTypeName() << "'" << LOG_END;
    }
    LOG_DEBUG(m_logger) << "Finalized arrow '" << GetName() << "'" << LOG_END;
}

