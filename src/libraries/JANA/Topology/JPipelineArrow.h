
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JMailbox.h>
#include <JANA/Topology/JPool.h>

template <typename DerivedT, typename MessageT>
class JPipelineArrow : public JArrow {
private:
    PlaceRef<MessageT> m_input {this, true, 1, 1};
    PlaceRef<MessageT> m_output {this, false, 1, 1};

public:
    JPipelineArrow(std::string name,
                   bool is_parallel,
                   bool is_source,
                   bool is_sink,
                   JMailbox<MessageT*>* input_queue,
                   JMailbox<MessageT*>* output_queue,
                   JPool<MessageT>* pool
                  )
        : JArrow(std::move(name), is_parallel, is_source, is_sink) {

        if (input_queue == nullptr) {
            assert(pool != nullptr);
            m_input.set_pool(pool);
        }
        else {
            m_input.set_queue(input_queue);
        }
        if (output_queue == nullptr) {
            assert(pool != nullptr);
            m_output.set_pool(pool);
        }
        else {
            m_output.set_queue(output_queue);
        }
    }

    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        Data<MessageT> in_data {location_id};
        Data<MessageT> out_data {location_id};

        bool success = m_input.pull(in_data) && m_output.pull(out_data);
        if (!success) {
            m_input.revert(in_data);
            m_output.revert(out_data);
            // TODO: Test that revert works properly
            
            auto end_total_time = std::chrono::steady_clock::now();
            result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
            return;
        }

        bool process_succeeded = true;
        JArrowMetrics::Status process_status = JArrowMetrics::Status::KeepGoing;
        assert(in_data.item_count == 1);
        MessageT* event = in_data.items[0];

        auto start_processing_time = std::chrono::steady_clock::now();
        static_cast<DerivedT*>(this)->process(event, process_succeeded, process_status);
        auto end_processing_time = std::chrono::steady_clock::now();

        if (process_succeeded) {
            in_data.item_count = 0;
            out_data.item_count = 1;
            out_data.items[0] = event;
        }
        m_input.push(in_data);
        m_output.push(out_data);

        // Publish metrics
        auto end_total_time = std::chrono::steady_clock::now();
        auto latency = (end_processing_time - start_processing_time);
        auto overhead = (end_total_time - start_total_time) - latency;
        result.update(process_status, process_succeeded, 1, latency, overhead);
    }
};
