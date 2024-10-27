// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Topology/JArrowMetrics.h"
#include <JANA/Topology/JTriggeredArrow.h>

class JFoldArrow : public JTriggeredArrow<JFoldArrow> {
public:
    const int CHILD_IN = 0;
    const int CHILD_OUT = 1;
    const int PARENT_OUT = 2;

private:
    // TODO: Support user-provided folders
    // JEventFolder* m_folder = nullptr;

    JEventLevel m_parent_level;
    JEventLevel m_child_level;

    Place m_child_in {this, true};
    Place m_child_out {this, false};
    Place m_parent_out {this, false};

public:
    JFoldArrow(
        std::string name,
        JEventLevel parent_level,
        JEventLevel child_level)

      : // m_folder(folder),
        m_parent_level(parent_level),
        m_child_level(child_level),
        m_child_in(this, true, 1, 1),
        m_child_out(this, false, 1, 1),
        m_parent_out(this, false, 1, 1)
    {
        set_name(name);
        m_next_input_port = CHILD_IN;
    }

    void attach_child_in(JMailbox<JEvent*>* child_in) {
        m_child_in.place_ref = child_in;
        m_child_in.is_queue = true;
    }

    void attach_child_out(JMailbox<JEvent*>* child_out) {
        m_child_out.place_ref = child_out;
        m_child_out.is_queue = true;
    }

    void attach_child_out(JEventPool* child_out) {
        m_child_out.place_ref = child_out;
        m_child_out.is_queue = false;
    }

    void attach_parent_out(JEventPool* parent_out) {
        m_parent_out.place_ref = parent_out;
        m_parent_out.is_queue = false;
    }

    void attach_parent_out(JMailbox<JEvent*>* parent_out) {
        m_parent_out.place_ref = parent_out;
        m_parent_out.is_queue = true;
    }

    void initialize() final {
        /*
        if (m_folder != nullptr) {
            m_folder->DoInit();
            LOG_INFO(m_logger) << "Initialized JEventFolder '" << m_unfolder->GetTypeName() << "'" << LOG_END;
        }
        else {
        }
        */
        LOG_INFO(m_logger) << "Initialized JEventFolder (trivial)" << LOG_END;
    }

    void finalize() final {
        /*
        if (m_folder != nullptr) {
            m_folder->DoFinish();
            LOG_INFO(m_logger) << "Finalized JEventFolder '" << m_unfolder->GetTypeName() << "'" << LOG_END;
        }
        */
        LOG_INFO(m_logger) << "Finalized JEventFolder (trivial)" << LOG_END;
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) {

        assert(m_next_input_port == CHILD_IN);

        // Check that child is at the correct event level
        if (event->GetLevel() != m_child_level) {
            throw JException("JFoldArrow received a child with the wrong event level");
        }

        // TODO: Call folders here
        // auto parent = child->GetParent(m_parent_level);
        // m_folder->Fold(*child, *parent);

        status = JArrowMetrics::Status::KeepGoing;
        outputs[0] = {event, CHILD_OUT};
        output_count = 1;

        auto* parent = event->ReleaseParent(m_parent_level);
        if (parent != nullptr) {
            // JEvent::ReleaseParent() returns nullptr if there are remaining references
            // to the parent event. If non-null, we are completely done with the parent
            // and are free to return it to the pool. In the future we could have the pool
            // itself handle the logic for releasing parents, in which case we could avoid
            // trivial JEventFolders.

            outputs[1] = {parent, PARENT_OUT};
            output_count = 2;
        }
    }

};


