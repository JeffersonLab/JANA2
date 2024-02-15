// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Engine/JArrow.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/Utils/JEventPool.h>

class JFoldArrow : public JArrow {
private:
    using EventT = std::shared_ptr<JEvent>;

    JEventFolder* m_folder = nullptr;

    PlaceRef<EventT> m_child_in;
    PlaceRef<EventT> m_child_out;
    PlaceRef<EventT> m_parent_out;

public:

    JFoldArrow(
        std::string name,
        JEventFolder* folder,
        JMailbox<EventT*>* child_in,
        JEventPool* child_out,
        JMailbox<EventT*>* parent_out)

      : JArrow(std::move(name), false, false, false), 
        m_folder(folder),
        m_child_in(this, child_in, true, 1, 1),
        m_child_out(this, child_out, false, 1, 1)
        m_parent_in(this, parent_out, false, 1, 1),
    {
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

    bool try_pull_all(Data<EventT>& ci, Data<EventT>& co, Data<EventT>& po) {
        bool success;
        success = m_parent_in.pull(ci);
        if (! success) {
            return false;
        }
        success = m_child_in.pull(co);
        if (! success) {
            return false;
        }
        success = m_child_out.pull(po);
        if (! success) {
            return false;
        }
        return true;
    }

    size_t push_all(Data<EventT>& ci, Data<EventT>& co, Data<EventT>& po) {
        size_t message_count = 0;
        message_count += m_child_in.push(ci);
        message_count += m_child_out.push(co);
        message_count += m_parent_out.push(po);
        return message_count;
    }

    void execute(JArrowMetrics& metrics, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();
        
        Data<EventT> child_in_data {location_id};
        Data<EventT> child_out_data {location_id};
        Data<EventT> parent_out_data {location_id};

        bool success = try_pull_all(child_in_data, child_out_data, parent_out_data);
        if (success) {

            auto start_processing_time = std::chrono::steady_clock::now();
            auto child = child_in_data.items[0];
            child_in_data.items[0] = nullptr;
            child_in_data.item_count = 0;
            // TODO: Decrement parent's ref count
            // TODO: Maybe fold should provide an identity for parent? What if no items make it past the filter?

            //auto status = m_folder->DoFold(*(child->get()), *(parent->get()));
            
            EventT* parent = child->GetParent();
            // TODO: How does fold know parent level?

            child_out_data.items[0] = child;
            child_out_data.item_count = 1;

            parent_out_data.items[0] = child;
            parent_out_data.item_count = 1;

            auto end_processing_time = std::chrono::steady_clock::now();
            size_t events_processed = push_all(parent_in_data, child_in_data, child_out_data);

            auto end_total_time = std::chrono::steady_clock::now();
            auto latency = (end_processing_time - start_processing_time);
            auto overhead = (end_total_time - start_total_time) - latency;

            metrics.update(JArrowMetrics::Status::KeepGoing, events_processed, 1, latency, overhead);
            return;
        }
        else {
            auto end_total_time = std::chrono::steady_clock::now();
            metrics.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
            return;
        }
    }

};


