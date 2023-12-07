
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>
#include <JANA/Engine/JPool.h>

template <typename DerivedT, typename MessageT>
class JPipelineArrow : public JArrow {
private:
    JMailbox<MessageT*>* m_input_queue;
    JMailbox<MessageT*>* m_output_queue;
    JPool<MessageT>* m_pool;

public:

    JPipelineArrow(std::string name,
                   bool is_parallel,
                   JArrow::NodeType node_type,
                   JMailbox<MessageT*>* input_queue,
                   JMailbox<MessageT*>* output_queue,
                   JPool<MessageT>* pool
                  )
        : JArrow(std::move(name), is_parallel, node_type),
          m_input_queue(input_queue),
          m_output_queue(output_queue),
          m_pool(pool) 
    {
    }

    size_t get_pending() final { return (m_input_queue == nullptr) ? 0 : m_input_queue->size(); };

    size_t get_threshold() final { return (m_input_queue == nullptr) ? 0 : m_input_queue->get_threshold(); }

    void set_threshold(size_t threshold) final { if (m_input_queue != nullptr) m_input_queue->set_threshold(threshold); }

    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        // ===================================
        // Reserve output before popping input
        // ===================================
        bool reserve_succeeded = true;
        if (m_output_queue != nullptr) {
            auto reserved_count = m_output_queue->reserve(1, location_id);
            reserve_succeeded = (reserved_count != 0);
        }
        if (!reserve_succeeded) {
            // Exit early!
            auto end_total_time = std::chrono::steady_clock::now();
            result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
            return;
        }

        // =========
        // Pop input
        // =========
        bool pop_succeeded = false;
        MessageT* event;
        if (m_input_queue == nullptr) {
            // Obtain from pool
            event = m_pool->get(location_id);
            pop_succeeded = (event != nullptr);
        }
        else {
            // Obtain from queue
            size_t popped_count = m_input_queue->pop_and_reserve(&event, 1, 1, location_id);
            pop_succeeded = (popped_count == 1);;

        }
        if (!pop_succeeded) {
            // Exit early!
            auto end_total_time = std::chrono::steady_clock::now();
            result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
            return;
        }


        // ========================
        // Process individual event
        // ========================

        auto start_processing_time = std::chrono::steady_clock::now();

        bool process_succeeded = true;
        JArrowMetrics::Status process_status = JArrowMetrics::Status::KeepGoing;
        static_cast<DerivedT*>(this)->process(event, process_succeeded, process_status);

        auto end_processing_time = std::chrono::steady_clock::now();


        // ==========
        // Push event
        // ==========
        if (process_succeeded) {
            // process() succeeded, so we push our event to the output queue/pool
            if (m_output_queue != nullptr) {
                // Push event to the output queue. This always succeeds due to reserve().
                m_output_queue->push_and_unreserve(&event, 1, 1, location_id);
                m_input_queue->unreserve(1, location_id);
            }
            else {
                // Push event to the output pool. This always succeeds.
                m_pool->put(event, location_id);
            }

        }
        else {
            // process() failed, so we return the event to the input queue/pool
            if (m_input_queue != nullptr) {
                // Return event to input queue. This always succeeds due to pop_and_reserve().
                m_input_queue->push_and_unreserve(&event, 1, 1, location_id);
            }
            else {
                // Return event to input pool. This always succeeds.
                m_pool->put(event, location_id);
            }
            if (m_output_queue != nullptr) {
                m_output_queue->unreserve(1, location_id);
            }
        }

        // Publish metrics
        auto end_total_time = std::chrono::steady_clock::now();
        auto latency = (end_processing_time - start_processing_time);
        auto overhead = (end_total_time - start_total_time) - latency;
        result.update(process_status, process_succeeded, 1, latency, overhead);
    }
};
