// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JGPUTapArrow.h"
#include <JANA/JEventProcessor.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEvent.h>


JGPUTapArrow::JGPUTapArrow(std::string name) {
    set_name(name);
    create_ports(1,1);
}

void JGPUTapArrow::fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) {

    LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << " for event# " << event->GetEventNumber() << LOG_END;

    // Obtain which proc to run from the event's continuation

    auto* continuation = event->GetSingle<JContinuation>(); // We have the option of tagging this

    // Run the fake proc 
    auto& fake_proc_result = continuation->factory_prefix

    // continuation->next_gpu_factory += 1;
  

    
    for (JEventProcessor* proc : m_procs) {
        JCallGraphEntryMaker cg_entry(*event->GetJCallGraphRecorder(), proc->GetTypeName()); // times execution until this goes out of scope
        if (proc->GetCallbackStyle() != JEventProcessor::CallbackStyle::LegacyMode) {
            proc->DoTap(*event);
        }
    }
    outputs[0] = {event, 1};
    output_count = 1;
    status = JArrow::FireResult::KeepGoing;
    LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " for event# " << event->GetEventNumber() << LOG_END;
}

void JGPUTapArrow::initialize() {
    LOG_DEBUG(m_logger) << "Initializing arrow '" << get_name() << "'" << LOG_END;
    LOG_DEBUG(m_logger) << "Initialized arrow '" << get_name() << "'" << LOG_END;
}

void JEventTapArrow::finalize() {
    LOG_DEBUG(m_logger) << "Finalizing arrow '" << get_name() << "'" << LOG_END;
    LOG_DEBUG(m_logger) << "Finalized arrow '" << get_name() << "'" << LOG_END;
}

