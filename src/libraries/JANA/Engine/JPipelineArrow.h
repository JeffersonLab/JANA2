
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Engine/JArrow.h>

template <typename DerivedT, typename MessageT>
class JPipelineArrow : public JArrow {
private:
    JMailbox<MessageT>* m_input_queue;
    JMailbox<MessageT>* m_output_queue;
    JPool<MessageT>* m_pool;

public:

    JPipelineArrow(std::string name,
                   bool is_parallel,
                   JArrow::NodeType node_type,
                   JMailbox<MessageT>* input_queue,
                   JMailbox<MessageT>* output_queue
                   JPool<MessageT>* pool
                  ) 
        : JArrow(std::move(name), is_parallel, node_type),
          m_input_queue(input_queue),
          m_output_queue(output_queue),
          m_pool(pool) 
    {
    }

    size_t get_pending() final { return m_input_queue->size(); };

    size_t get_threshold() final { return m_input_queue->get_threshold(); }

    void set_threshold(size_t) final { m_input_queue->set_threshold(threshold); }

    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        MessageT x; // Please be some kind of pointer type
        bool success;
        auto in_status = m_input_queue->pop(x, success, location_id);

        auto start_latency_time = std::chrono::steady_clock::now();
        if (success) {
            static_cast<DerivedT*>(this)->process(x);
        }
        auto end_latency_time = std::chrono::steady_clock::now();

        auto out_status = EventQueue::Status::Ready;

        if (success) {
            out_status = m_output_queue->push(x, location_id);
        }
        auto end_queue_time = std::chrono::steady_clock::now();

        JArrowMetrics::Status status;
        if (in_status == EventQueue::Status::Empty) {
            status = JArrowMetrics::Status::ComeBackLater;
        }
        else if (in_status == EventQueue::Status::Ready && out_status == EventQueue::Status::Ready) {
            status = JArrowMetrics::Status::KeepGoing;
        }
        else {
            status = JArrowMetrics::Status::ComeBackLater;
        }
        auto latency = (end_latency_time - start_latency_time);
        auto overhead = (end_queue_time - start_total_time) - latency;
        result.update(status, success, 1, latency, overhead);
    }
};
