// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Topology/JArrowMetrics.h"
#include <JANA/Topology/JTriggeredArrow.h>
#include <JANA/JEventUnfolder.h>

class JUnfoldArrow : public JTriggeredArrow<JUnfoldArrow> {
public:
    const int PARENT_IN = 0;
    const int CHILD_IN = 1;
    const int CHILD_OUT = 2;

private:
    JEventUnfolder* m_unfolder = nullptr;
    JEvent* m_parent_event = nullptr;
    JEvent* m_child_event = nullptr;

    Place m_parent_in {this, true};
    Place m_child_in {this, true};
    Place m_child_out {this, false};

public:
    JUnfoldArrow(std::string name, JEventUnfolder* unfolder) : m_unfolder(unfolder) {
        set_name(name);
        m_next_input_port = PARENT_IN;
    }

    void attach_parent_in(JMailbox<JEvent*>* parent_in) {
        m_parent_in.place_ref = parent_in;
        m_parent_in.is_queue = true;
    }

    void attach_child_in(JEventPool* child_in) {
        m_child_in.place_ref = child_in;
        m_child_in.is_queue = false;
    }

    void attach_child_in(JMailbox<JEvent*>* child_in) {
        m_child_in.place_ref = child_in;
        m_child_in.is_queue = true;
    }

    void attach_child_out(JMailbox<JEvent*>* child_out) {
        m_child_out.place_ref = child_out;
        m_child_out.is_queue = true;
    }


    void initialize() final {
        m_unfolder->DoInit();
        LOG_INFO(m_logger) << "Initialized JEventUnfolder '" << m_unfolder->GetTypeName() << "'" << LOG_END;
    }

    void finalize() final {
        m_unfolder->DoFinish();
        LOG_INFO(m_logger) << "Finalized JEventUnfolder '" << m_unfolder->GetTypeName() << "'" << LOG_END;
    }

    size_t get_pending() final {
        size_t sum = 0;
        for (Place* place : m_places) {
            if (place->is_input && place->is_queue) {
                auto queue = static_cast<JMailbox<JEvent*>*>(place->place_ref);
                sum += queue->size();
            }
        }
        if (m_parent_event != nullptr) {
            sum += 1; 
            // Handle the case of UnfoldArrow hanging on to a parent
        }
        return sum;
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) {

        // Take whatever we were given
        if (this->m_next_input_port == PARENT_IN) {
            assert(m_parent_event == nullptr);
            m_parent_event = event;
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
            status = JArrowMetrics::Status::KeepGoing;
            return;
        }

        // Check if we should exit early because we don't have a child event
        if (m_child_event == nullptr) {
            m_next_input_port = CHILD_IN;
            output_count = 0;
            status = JArrowMetrics::Status::KeepGoing;
            return;
        }

        // At this point we know we have both inputs, so we can run the unfolder. First we validate 
        // that the events we received are at the correct level for the unfolder. Hopefully the only 
        // way to end up here is to override the JTopologyBuilder wiring and do it wrong

        if (m_parent_event->GetLevel() != m_unfolder->GetLevel()) {
            throw JException("JUnfolder: Expected parent with level %d, got %d", m_unfolder->GetLevel(), m_parent_event->GetLevel());
        }

        if (m_child_event->GetLevel() != m_unfolder->GetChildLevel()) {
            throw JException("JUnfolder: Expected child with level %d, got %d", m_unfolder->GetChildLevel(), m_child_event->GetLevel());
        }

        auto result = m_unfolder->DoUnfold(*m_parent_event, *m_child_event);
        LOG_DEBUG(m_logger) << "Unfold succeeded: Parent event = " << m_parent_event->GetEventNumber() << ", child event = " << m_child_event->GetEventNumber() << LOG_END;

        if (result == JEventUnfolder::Result::KeepChildNextParent) {
            m_parent_event->Release(); // Decrement the reference count so that this can be recycled
            m_parent_event = nullptr;
            output_count = 0;
            m_next_input_port = PARENT_IN;
            status = JArrowMetrics::Status::KeepGoing;
            LOG_DEBUG(m_logger) << "Unfold finished with parent event = " << m_parent_event->GetEventNumber() << LOG_END;
            return;
        }
        else if (result == JEventUnfolder::Result::NextChildKeepParent) {
            m_child_event->SetParent(m_parent_event);
            outputs[0] = {m_child_event, CHILD_OUT};
            output_count = 1;
            m_child_event = nullptr;
            m_next_input_port = CHILD_IN;
            status = JArrowMetrics::Status::KeepGoing;
            return;
        }
        else if (result == JEventUnfolder::Result::NextChildNextParent) {
            m_child_event->SetParent(m_parent_event);
            m_parent_event->Release(); // Decrement the reference count so that this can be recycled
            outputs[0] = {m_child_event, CHILD_OUT};
            output_count = 1;
            m_child_event = nullptr;
            m_parent_event = nullptr;
            m_next_input_port = PARENT_IN;
            status = JArrowMetrics::Status::KeepGoing;
            LOG_DEBUG(m_logger) << "Unfold finished with parent event = " << m_parent_event->GetEventNumber() << LOG_END;
            return;
        }
        else {
            throw JException("Unsupported (corrupt?) JEventUnfolder::Result");
        }
    }
};


