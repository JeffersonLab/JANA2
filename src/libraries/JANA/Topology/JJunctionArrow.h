// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrow.h>
#include <JANA/Topology/JMailbox.h>
#include <JANA/Topology/JPool.h>



template <typename DerivedT>
class JJunctionArrow : public JArrow {

protected:    
    Place first_input {this, true, 1, 1};
    Place first_output {this, false, 1, 1};
    Place second_input {this, true, 1, 1};
    Place second_output {this, false, 1, 1};

public:
    using Status = JArrowMetrics::Status;

    JJunctionArrow(std::string name,
                   bool is_parallel,
                   bool is_source,
                   bool is_sink
                  )
        : JArrow(std::move(name), is_parallel, is_source, is_sink)
    {
    }

    bool try_pull_all(Data& fi, Data& fo, Data& si, Data& so) {

        bool success;
        success = first_input.pull(fi);
        if (! success) {
            return false;
        }
        success = first_output.pull(fo);
        if (! success) {
            first_input.revert(fi);
            return false;
        }
        success = second_input.pull(si);
        if (! success) {
            first_input.revert(fi);
            first_output.revert(fo);
            return false;
        }
        success = second_output.pull(so);
        if (! success) {
            first_input.revert(fi);
            first_output.revert(fo);
            second_input.revert(si);
            return false;
        }
        return true;
    }

    size_t push_all(Data& fi, Data& fo, Data& si, Data& so) {
        size_t message_count = 0;
        message_count += first_input.push(fi);
        message_count += first_output.push(fo);
        message_count += second_input.push(si);
        message_count += second_output.push(so);
        return message_count;
    }

    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        Data first_input_data {location_id};
        Data first_output_data {location_id};
        Data second_input_data {location_id};
        Data second_output_data {location_id};

        bool success = try_pull_all(first_input_data, first_output_data, second_input_data, second_output_data);
        if (success) {

            auto start_processing_time = std::chrono::steady_clock::now();
            auto process_status = static_cast<DerivedT*>(this)->process(first_input_data, first_output_data, second_input_data, second_output_data);
            auto end_processing_time = std::chrono::steady_clock::now();
            size_t events_processed = push_all(first_input_data, first_output_data, second_input_data, second_output_data);

            auto end_total_time = std::chrono::steady_clock::now();
            auto latency = (end_processing_time - start_processing_time);
            auto overhead = (end_total_time - start_total_time) - latency;
            result.update(process_status, events_processed, 1, latency, overhead);
            return;
        }
        else {
            auto end_total_time = std::chrono::steady_clock::now();
            result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
            return;
        }
    }
};


