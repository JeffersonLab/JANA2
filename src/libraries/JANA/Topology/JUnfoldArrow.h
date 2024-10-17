// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrow.h>
#include <JANA/JEventUnfolder.h>

class JUnfoldArrow : public JArrow {
private:

    JEventUnfolder* m_unfolder = nullptr;
    EventT* m_parent_event = nullptr;
    bool m_ready_to_fetch_parent = true;

    Place m_parent_in;
    Place m_child_in;
    Place m_child_out;

public:

    JUnfoldArrow(
        std::string name,
        JEventUnfolder* unfolder)

      : JArrow(std::move(name), false, false, false), 
        m_unfolder(unfolder),
        m_parent_in(this, true, 1, 1),
        m_child_in(this, true, 1, 1),
        m_child_out(this, false, 1, 1)
    {
    }


    void attach_parent_in(JMailbox<EventT*>* parent_in) {
        m_parent_in.place_ref = parent_in;
        m_parent_in.is_queue = true;
    }

    void attach_child_in(JEventPool* child_in) {
        m_child_in.place_ref = child_in;
        m_child_in.is_queue = false;
    }

    void attach_child_in(JMailbox<EventT*>* child_in) {
        m_child_in.place_ref = child_in;
        m_child_in.is_queue = true;
    }

    void attach_child_out(JMailbox<EventT*>* child_out) {
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

    bool try_pull_all(Data& pi, Data& ci, Data& co) {
        bool success;
        success = m_parent_in.pull(pi);
        if (! success) {
            return false;
        }
        success = m_child_in.pull(ci);
        if (! success) {
            m_parent_in.push(pi);
            return false;
        }
        success = m_child_out.pull(co);
        if (! success) {
            m_parent_in.push(pi);
            m_child_in.push(ci);
            return false;
        }
        return true;
    }

    size_t push_all(Data& parent_in, Data& child_in, Data& child_out) {
        size_t message_count = 0;
        message_count += m_parent_in.push(parent_in);
        message_count += m_child_in.push(child_in);
        message_count += m_child_out.push(child_out);
        return message_count;
    }


    size_t get_pending() final {
        size_t sum = 0;
        for (Place* place : m_places) {
            sum += place->get_pending();
        }
        if (m_parent_event != nullptr) {
            sum += 1; 
            // Handle the case of UnfoldArrow hanging on to a parent
        }
        return sum;
    }


    void execute(JArrowMetrics& metrics, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        Data parent_in_data {location_id};
        Data child_in_data {location_id};
        Data child_out_data {location_id};

        bool success = try_pull_all(parent_in_data, child_in_data, child_out_data);
        if (success) {

            auto start_processing_time = std::chrono::steady_clock::now();
            if (m_ready_to_fetch_parent) {
                m_ready_to_fetch_parent = false;
                m_parent_in.min_item_count = 0;
                m_parent_in.max_item_count = 0;
                m_parent_event = parent_in_data.items[0];
                parent_in_data.items[0] = nullptr;
                parent_in_data.item_count = 0;
            }
            auto child = child_in_data.items[0];
            child_in_data.items[0] = nullptr;
            child_in_data.item_count = 0;
            if (m_parent_event == nullptr) {
                throw JException("Attempting to unfold without a valid parent event");
            }
            if (m_parent_event->get()->GetLevel() != m_unfolder->GetLevel()) {
                throw JException("JUnfolder: Expected parent with level %d, got %d", m_unfolder->GetLevel(), m_parent_event->get()->GetLevel());
            }
            if (child->get()->GetLevel() != m_unfolder->GetChildLevel()) {
                throw JException("JUnfolder: Expected child with level %d, got %d", m_unfolder->GetChildLevel(), child->get()->GetLevel());
            }
            
            auto status = m_unfolder->DoUnfold(*(m_parent_event->get()), *(child->get()));


            // Join always succeeds (for now)
            child->get()->SetParent(m_parent_event);

            LOG_DEBUG(m_logger) << "Unfold succeeded: Parent event = " << m_parent_event->get()->GetEventNumber() << ", child event = " << child->get()->GetEventNumber() << LOG_END;
            // TODO: We'll need something more complicated for the streaming join case

            if (status == JEventUnfolder::Result::NextChildNextParent || status == JEventUnfolder::Result::KeepChildNextParent) {
                LOG_DEBUG(m_logger) << "Unfold finished with parent event = " << m_parent_event->get()->GetEventNumber() << LOG_END;
                m_ready_to_fetch_parent = true;
                m_parent_event->get()->Release();
                m_parent_event = nullptr;
                m_parent_in.min_item_count = 1;
                m_parent_in.max_item_count = 1;
            }

            child_out_data.items[0] = child;
            child_out_data.item_count = 1;

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


