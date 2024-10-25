
#pragma once

#include <JANA/Topology/JArrow.h>

template <typename DerivedT>
struct JTriggeredArrow : public JArrow {


    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        JEvent* input = pull(m_next_input_port, location_id);

        if (input == nullptr) {
            // Failed to obtain the input we needed; arrow is NOT ready to fire

            auto end_total_time = std::chrono::steady_clock::now();

            result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
        }
        else {
            // Obtained the input we needed; arrow is ready to fire

            auto start_processing_time = std::chrono::steady_clock::now();

            OutputData outputs;
            size_t output_count;
            JArrowMetrics::Status process_status = JArrowMetrics::Status::KeepGoing;

            static_cast<DerivedT*>(this)->fire(input, outputs, output_count, process_status);

            auto end_processing_time = std::chrono::steady_clock::now();

            push(outputs, output_count, location_id);

            auto end_total_time = std::chrono::steady_clock::now();

            auto latency = (end_processing_time - start_processing_time);
            auto overhead = (end_total_time - start_total_time) - latency;
            result.update(process_status, 1, 1, latency, overhead);
        }
    }
};


