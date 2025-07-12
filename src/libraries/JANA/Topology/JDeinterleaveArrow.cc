
#include "JANA/Topology/JArrow.h"
#include <JANA/Topology/JDeinterleaveArrow.h>

JDeinterleaveArrow::JDeinterleaveArrow() = default;
JDeinterleaveArrow::~JDeinterleaveArrow() = default;

void JDeinterleaveArrow::SetLevels(std::vector<JEventLevel> levels) {
    // TODO: The JMultilevelArrow abstraction isn't quite right because Style is required to be OneToAll, so as a user
    // we have to know to ignore ConfigurePorts and use SetLevels() instead
    ConfigurePorts(Style::OneToAll, levels);
    m_child_event_level = levels.back();
}

void JDeinterleaveArrow::Process(JEvent* input, std::vector<JEvent*>& outputs, JEventLevel&, FireResult& result) {

    if (input->GetLevel() == m_child_event_level) {
        // Attach all existing parents and immediately forward
        // We are NOT calling ReleaseRefToSelf(), which is probably not compatible with the existing JTopologyBuilder
        for (auto [level, parent] : m_pending_parents) {
            input->SetParent(parent);
            // Note that this only attaches parents that we already, so if the parents arrive in the wrong order they
            // will just be missing. If this is expected behavior, you'll need to set your downstream parent inputs to be optional.
        }
        outputs.push_back(input);
    }
    else {
        // This is a parent event, so evict the previous parent event and forward to the REJECTED_PARENTS_OUT port :) 
        // TODO: Note that if our data stream contained two consecutive parents of the same level (e.g. "CC" in "PPPRCPPPCC"), 
        // the parent will be evicted with zero children. This won't work with how JEventFolder is currently
        // set up -- we need to change JEventFolder to directly accept parent events as well. For now we simply assume that
        // our input stream is structured correctly, e.g. (R(CP+)+)+. We also need the assumption that the final child event
        // _knows_ it is the final child event.

        auto it = m_pending_parents.find(input->GetLevel());
        if (it != m_pending_parents.end()) {
            // There IS an old parent
            JEvent* old_parent = it->second;
            it->second = input;
            outputs.push_back(old_parent);
        }
        else {
            m_pending_parents[input->GetLevel()] = input;
        }
    }
    // Result is always KeepGoing
    result = FireResult::KeepGoing;

    // TODO: Eventual problem if we ever use JEventFolder: Who triggers the release of pending parents after source 
    // or unfolder declares itself finished?
    // ExecutionEngine will never call DeinterleaveArrow again because Deinterleave is waiting for an 
    // input event that is never coming. This means Fold() and Interleave() will never see the last parents.
    // Pending events need to be released in the order of the topology shutting down. Maybe this means bringing back JEventQueue::is_finished...
    // Or we can attempt to send some kind of tombstone event from each source or unfolder. Or we ONLY use DeinterleaveArrow with
    // sources or unfolders, in which case we can directly observe that the user component in fact returned Finished. 
    // (Note this solves the problem with DeinterleaveArrow but doesn't solve the same problem
    // with JEventFolder when the parent source shuts off after the last parent event has already finished processing)

};

