
#include <JANA/Topology/JArrow.h>


class Deinterleave : public JArrow {
public:
    enum PortIndex {EVENT_IN=0, EVENT_OUT=1};

private:
    JEventLevel m_child_level = JEventLevel::PhysicsEvent;
    std::map<JEventLevel, JEvent*> m_pending_events;

public:
    Deinterleave() {
        set_name("Deinterleaver");
        create_ports(1, 1);
    }
    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) {
        if (input->GetLevel() == m_child_level) {
            // Attach all parents and emit child
            output_count = 1;
            outputs[0] = {input, EVENT_OUT};
            // TODO: Attach parents to input
        }
        else {
            // Swap out a parent
            // The tricky part is figuring out what to do with the old one
            JEvent* old_parent = nullptr;
            auto children_in_flight = old_parent->Release();

            if (old_parent->has_children) {
                // Folder will take care of passing on the parent

                if (children_in_flight == 0) {
                    // There's a problem with this design where the Folder has no way of knowing
                    // that there's no more children coming for a given parent
                }
                else {
                    // Folder will do the right thing
                }
            }
            else {
                // old_parent is "rejected" and goes straight to the ParentMap/Tap
            }

            if (old_parent->Release() == 0) {
                // Handle the case where children have been emitted with a reference
                // to the parent being ejected
                //
                // Consider the case where a Folder has consumed all previous children 
                // and there are no more children coming...
                // In that case, there's no one to turn off the lights

            }
            else {
                // Handle the case where children have already been emitted, and they 
                // HAVEN'T all been folded away yet
            }
            output_count = 0;
        }
        status = FireResult::KeepGoing;
    }
};



