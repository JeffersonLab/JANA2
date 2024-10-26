
#pragma once

#include <JANA/Topology/JArrow.h>
#include <chrono>

template <typename DerivedT>
struct JTriggeredArrow : public JArrow {

    using clock_t = std::chrono::steady_clock;

    int m_next_input_port=0; // -1 denotes "no input necessary", e.g. for barrier events
    clock_t::time_point m_next_visit_time=clock_t::now();


    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        if (m_next_visit_time > start_total_time) {
            // If we haven't reached the next visit time, exit immediately
            result.update(JArrowMetrics::Status::ComeBackLater, 0, 0, std::chrono::milliseconds(0), std::chrono::milliseconds(0));
            return;
        }

        JEvent* input = nullptr;
        if (m_next_input_port != -1) {
            input = pull(m_next_input_port, location_id);
        }

        if (input == nullptr && m_next_input_port != -1) {
            // Failed to obtain the input we needed; arrow is NOT ready to fire

            auto end_total_time = std::chrono::steady_clock::now();

            result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
        }
        else {
            // Obtained the input we needed; arrow is ready to fire
            // Remember that `input` might be nullptr, in case arrow doesn't need any input event

            auto start_processing_time = std::chrono::steady_clock::now();

            OutputData outputs;
            size_t output_count;
            JArrowMetrics::Status status = JArrowMetrics::Status::KeepGoing;

            static_cast<DerivedT*>(this)->fire(input, outputs, output_count, status);

            auto end_processing_time = std::chrono::steady_clock::now();

            push(outputs, output_count, location_id);

            auto end_total_time = std::chrono::steady_clock::now();

            auto latency = (end_processing_time - start_processing_time);
            auto overhead = (end_total_time - start_total_time) - latency;
            result.update(status, 1, 1, latency, overhead);
        }
    }
};


