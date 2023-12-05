// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>
#include <JANA/Engine/JPool.h>

template <typename T, size_t BufSize>
struct Data {
    std::array<T*, BufSize> items;
    size_t item_count;
    size_t reserve_count;
    size_t location_id;
}

template <typename T>
struct PlaceRef {
    JMailbox<T*>* queue;
    JPool<T>* pool;
    bool is_input;
    size_t min_item_count;
    size_t max_item_count;

    bool pull(Data& data) {
        if (is_input) { // Actually pull the data
            if (queue != nullptr) {
                data.item_count = queue->pop_and_reserve(data.items.data(), min_item_count, max_item_count, data.location_id);
                data.reserve_count = data.item_count;
                return (data.item_count >= min_item_count);
            }
            else {
                data.item_count = pool->pop(data.items.data(), min_item_count, max_item_count, data.location_id);
                data.reserve_count = 0;
                return (data.item_count >= min_item_count);
            }
        }
        else {
            if (queue != nullptr) {
                // Reserve a space on the output queue
                data.item_count = 0;
                data.reserve_count = m_output_queue->reserve(min_item_count, max_item_count, data.location_id);
                return (data.reserve_count >= min_item_count);
            }
            else {
                // No need to reserve on pool -- either there is space or limit_events_in_flight=false
                data.item_count = 0;
                data.reserve_count = 0;
                return true;
            }
        }
    }

    void revert(Data& data, size_t location_id) {
        if (queue != nullptr) {
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
        }
        else {
            if (is_input) {
                pool->push(data.items.data(), data.item_count, data.location_id);
            }
        }
    }

    size_t push(Data& data) {
        if (queue != nullptr) {
            queue->push_and_unreserve(data.items.data(), data.item_count, data.reserve_count, data.location_id);
            return is_input ? 0 : data.item_count;
        }
        else {
            pool.push(data.items.data(), data.item_count, data.location_id);
            return 0;
        }
    }
}

template <typename DerivedT, typename FirstT, typename SecondT, size_t BufSize=40>
class JJunctionArrow : public JArrow {
    
    PlaceRef<FirstT> first_input;
    PlaceRef<FirstT> first_output;
    PlaceRef<SecondT> second_input;
    PlaceRef<SecondT> second_output;

public:
    JJunctionArrow(std::string name,
                   bool is_parallel,
                   JArrow::NodeType node_type,
                  )
        : JArrow(std::move(name), is_parallel, node_type)
    {
    }

    size_t get_pending() final { 
        // This is actually used by JScheduler for better or for worse
        size_t first_pending = first_input.queue ? first_input.queue->size() : 0;
        size_t second_pending = second_input.queue ? second_input.queue->size() : 0;
        return first_pending + second_pending;
    };

    size_t get_threshold() final { 
        // TODO: Is this even meaningful? Only used in JArrowSummary I think -- Maybe get rid of this eventually?
        return 0;
    }

    void set_threshold(size_t threshold) final {  }


    bool try_pull_all(Data<FirstT>& fi, Data<FirstT>& fo, Data<SecondT>& si, Data<SecondT>& so) {

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

    size_t push_all(Data<FirstT>& fi, Data<FirstT>& fo, Data<SecondT>& si, Data<SecondT>& so) {
        size_t message_count = 0;
        message_count += first_input.push(fi);
        message_count += first_output.push(fo);
        message_count += second_input.push(si);
        message_count += second_output.push(so);
        return message_count;
    }

    void execute(JArrowMetrics& result, size_t location_id) final {

        auto start_total_time = std::chrono::steady_clock::now();

        Data<FirstT> first_input_data;
        Data<FirstT> first_output_data;
        Data<SecondT> second_input_data;
        Data<SecondT> second_output_data;

        bool success = try_pull_all(first_input_data, first_output_data, second_input_data, second_output_data, location_id);
        if (success) {

            auto start_processing_time = std::chrono::steady_clock::now();
            auto process_status = static_cast<DerivedT*>(this)->process(first_input_data, first_output_data, second_input_data, second_output_data);
            auto end_processing_time = std::chrono::steady_clock::now();
            size_t events_processed = push_all(first_input_data, first_output_data, second_input_data, second_output_data, location_id);

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


