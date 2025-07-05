// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrow.h>
#include <JANA/JEventUnfolder.h>

class JUnfoldArrow : public JArrow {
public:
    enum PortIndex {PARENT_IN=0, CHILD_IN=1, CHILD_OUT=2, REJECTED_PARENT_OUT=3};

private:
    JEventUnfolder* m_unfolder = nullptr;
    JEvent* m_parent_event = nullptr;
    JEvent* m_child_event = nullptr;

public:
    JUnfoldArrow(std::string name, JEventUnfolder* unfolder) : m_unfolder(unfolder) {
        set_name(name);
        create_ports(2, 2);
        m_next_input_port = PARENT_IN;
    }

    void initialize() final {
        m_unfolder->DoInit();
        LOG_INFO(m_logger) << "Initialized JEventUnfolder '" << m_unfolder->GetTypeName() << "'" << LOG_END;
    }

    void finalize() final {
        m_unfolder->DoFinish();
        LOG_INFO(m_logger) << "Finalized JEventUnfolder '" << m_unfolder->GetTypeName() << "'" << LOG_END;
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) final {

        // Take whatever we were given
        if (this->m_next_input_port == PARENT_IN) {
            assert(m_parent_event == nullptr);
            m_parent_event = event;
            m_parent_event->TakeRefToSelf();
        }
        else if (this->m_next_input_port == CHILD_IN) {
            assert(m_child_event == nullptr);
            m_child_event = event;
        }
        else {
            throw JException("Invalid input port for JEventUnfolder!");
        }

        // Check if we should exit early because we don't have a parent event
        if (m_parent_event == nullptr) {
            m_next_input_port = PARENT_IN;
            output_count = 0;
            status = JArrow::FireResult::KeepGoing;
            return;
        }

        // Check if we should exit early because we don't have a child event
        if (m_child_event == nullptr) {
            m_next_input_port = CHILD_IN;
            output_count = 0;
            status = JArrow::FireResult::KeepGoing;
            return;
        }

        // At this point we know we have both inputs, so we can run the unfolder. First we validate 
        // that the events we received are at the correct level for the unfolder. Hopefully the only 
        // way to end up here is to override the JTopologyBuilder wiring and do it wrong

        if (m_parent_event->GetLevel() != m_unfolder->GetLevel()) {
            throw JException("JUnfolder: Expected parent with level %s, got %s", toString(m_unfolder->GetLevel()).c_str(), toString(m_parent_event->GetLevel()).c_str());
        }

        if (m_child_event->GetLevel() != m_unfolder->GetChildLevel()) {
            throw JException("JUnfolder: Expected child with level %s, got %s", toString(m_unfolder->GetChildLevel()).c_str(), toString(m_child_event->GetLevel()).c_str());
        }

        auto result = m_unfolder->DoUnfold(*m_parent_event, *m_child_event);
        LOG_DEBUG(m_logger) << "Unfold succeeded: Parent event = " << m_parent_event->GetEventNumber() << ", child event = " << m_child_event->GetEventNumber() << LOG_END;

        if (result == JEventUnfolder::Result::KeepChildNextParent) {
            // KeepChildNextParent is a little more complicated because we have to handle the case of the parent having no children.
            // In this case the parent obviously doesn't get shared among any children, and instead it is sent to the REJECTED_PARENT_OUT port.
            int child_count = m_parent_event->ReleaseRefToSelf(); // Decrement the reference count so that this can be recycled
            LOG_DEBUG(m_logger) << "Unfold finished with parent event = " << m_parent_event->GetEventNumber() << " (" << child_count << " children emitted)";

            if (child_count > 0) {
                // Parent DOES have children even though this particular child isn't one of them
                m_parent_event = nullptr;
                output_count = 0;
                m_next_input_port = PARENT_IN;
                status = JArrow::FireResult::KeepGoing;
                return;
            }
            else {
                // Parent has NO children
                output_count = 1;
                outputs[0] = {m_parent_event, REJECTED_PARENT_OUT};
                m_parent_event = nullptr;
                m_next_input_port = PARENT_IN;
                status = JArrow::FireResult::KeepGoing;
                return;
            }
        }
        else if (result == JEventUnfolder::Result::NextChildKeepParent) {
            m_child_event->SetParent(m_parent_event);
            outputs[0] = {m_child_event, CHILD_OUT};
            output_count = 1;
            m_child_event = nullptr;
            m_next_input_port = CHILD_IN;
            status = JArrow::FireResult::KeepGoing;
            return;
        }
        else if (result == JEventUnfolder::Result::NextChildNextParent) {
            m_child_event->SetParent(m_parent_event);
            m_parent_event->ReleaseRefToSelf(); // Decrement the reference count so that this can be recycled
            outputs[0] = {m_child_event, CHILD_OUT};
            output_count = 1;
            LOG_DEBUG(m_logger) << "Unfold finished with parent event = " << m_parent_event->GetEventNumber() << LOG_END;
            m_child_event = nullptr;
            m_parent_event = nullptr;
            m_next_input_port = PARENT_IN;
            status = JArrow::FireResult::KeepGoing;
            return;
        }
        else {
            throw JException("Unsupported (corrupt?) JEventUnfolder::Result");
        }
    }
};


