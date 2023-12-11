// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>
#include <JANA/Engine/JPool.h>


template <typename DerivedT, typename... PlaceTs>
class JOmniArrow : public JArrow {

protected:
    [[maybe_unused]]
    std::tuple<PlaceRef<PlaceTs>...> places {(sizeof(PlaceTs), this)...};

public:
    using Status = JArrowMetrics::Status;

    JOmniArrow(std::string name,
               bool is_parallel,
               bool is_source,
               bool is_sink
              )
        : JArrow(std::move(name), is_parallel, is_source, is_sink)
    {
    }


    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();
        // Create data holders
        [[maybe_unused]]
        std::tuple<Data<PlaceTs>...> data {(sizeof(PlaceTs), location_id)...};

        /*
        // Attempt to pull from all places
        bool success = (std::get<PlaceRef<PlaceTs>>(places).pull(std::get<Data<PlaceTs>>(data)) && ...);
        if (!success) {

            // Revert all PlaceRefs
            (std::get<PlaceRef<PlaceTs>>(places).revert(std::get<Data<PlaceTs>>(data)), ...);

            // Report back and exit
            auto end_total_time = std::chrono::steady_clock::now();
            result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
        }
        else {

            auto start_processing_time = std::chrono::steady_clock::now();

            // Call user-provided process() given fully typed Data
            auto process_status = static_cast<DerivedT*>(this)->process(data);

            auto end_processing_time = std::chrono::steady_clock::now();

            // Push to all places (always succeeds, assuming user didn't muck with reserved_count)
            size_t events_processed = (std::get<PlaceRef<PlaceTs>>(places).push(std::get<Data<PlaceTs>>(data)) + ...);

            auto end_total_time = std::chrono::steady_clock::now();
            auto latency = (end_processing_time - start_processing_time);
            auto overhead = (end_total_time - start_total_time) - latency;
            result.update(process_status, events_processed, 1, latency, overhead);
        }
        */
    }
};


