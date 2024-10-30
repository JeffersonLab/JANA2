// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JEventFolder.h>
#include <JANA/Topology/JArrow.h>

class JFoldArrow : public JArrow {
public:
    enum PortIndex {CHILD_IN=0, CHILD_OUT=1, PARENT_OUT=2};

private:
    JEventFolder* m_folder = nullptr;

    JEventLevel m_parent_level;
    JEventLevel m_child_level;

public:
    JFoldArrow(
        std::string name,
        JEventLevel parent_level,
        JEventLevel child_level)

      : m_parent_level(parent_level),
        m_child_level(child_level)
    {
        set_name(name);
        create_ports(1, 2);
        m_next_input_port = CHILD_IN;
    }

    void set_folder(JEventFolder* folder) {
        m_folder = folder;
    }

    void initialize() final {
        if (m_folder != nullptr) {
            m_folder->DoInit();
            LOG_INFO(m_logger) << "Initialized JEventFolder '" << m_folder->GetTypeName() << "'" << LOG_END;
        }
        else {
            LOG_INFO(m_logger) << "Initialized JEventFolder (trivial)" << LOG_END;
        }
    }

    void finalize() final {
        if (m_folder != nullptr) {
            m_folder->DoFinish();
            LOG_INFO(m_logger) << "Finalized JEventFolder '" << m_folder->GetTypeName() << "'" << LOG_END;
        }
        else {
            LOG_INFO(m_logger) << "Finalized JEventFolder (trivial)" << LOG_END;
        }
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) final {

        assert(m_next_input_port == CHILD_IN);

        // Check that child is at the correct event level
        if (event->GetLevel() != m_child_level) {
            throw JException("JFoldArrow received a child with the wrong event level");
        }

        if (m_folder != nullptr) {
            auto parent = const_cast<JEvent*>(&event->GetParent(m_parent_level));
            m_folder->DoFold(*event, *parent);
        }

        status = JArrowMetrics::Status::KeepGoing;
        outputs[0] = {event, CHILD_OUT};
        output_count = 1;

        auto* released_parent = event->ReleaseParent(m_parent_level);
        if (released_parent != nullptr) {
            // JEvent::ReleaseParent() returns nullptr if there are remaining references
            // to the parent event. If non-null, we are completely done with the parent
            // and are free to return it to the pool. In the future we could have the pool
            // itself handle the logic for releasing parents, in which case we could avoid
            // trivial JEventFolders.

            outputs[1] = {released_parent, PARENT_OUT};
            output_count = 2;
        }
    }

};


