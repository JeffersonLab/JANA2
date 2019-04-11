//
// Created by nbrei on 4/8/19.
//

#include <JANA/JEventSourceArrow.h>

using SourceStatus = JEventSource::RETURN_STATUS;
using ArrowStatus = JEventSourceArrow::Status;

JEventSourceArrow::JEventSourceArrow(std::string name, JEventSource& source, EventQueue* output_queue)
    : JArrow(name, false)
    , _source(source)
    , _output_queue(output_queue) {

    _output_queue->attach_upstream(this);
    attach_downstream(_output_queue);
}

JArrow::Status JEventSourceArrow::execute() {

    if (!is_active()) {
        return JArrow::Status::Finished;
    }
    if (!_is_initialized) {
        _source.Open();
        _is_initialized = true;
    }

    SourceStatus in_status = SourceStatus::kSUCCESS;
    auto start_time = std::chrono::steady_clock::now();
    try {
        size_t item_count = get_chunksize();
        while (item_count-- != 0) {
            _chunk_buffer.push_back(_source.GetEvent());
        }
    }
    catch (SourceStatus rs) {
        in_status = rs;
    }
    catch (...) {
        in_status = SourceStatus::kERROR;
    }
    auto latency_time = std::chrono::steady_clock::now();
    auto message_count = _chunk_buffer.size();
    auto out_status = _output_queue->push(_chunk_buffer);
    _chunk_buffer.clear();
    auto finished_time = std::chrono::steady_clock::now();

    auto latency = latency_time - start_time;
    auto overhead = finished_time - latency_time;
    update_metrics(message_count, 1, latency, overhead);

    if (in_status == SourceStatus::kNO_MORE_EVENTS) {
        // There should be a _source.Close() of some kind
        set_upstream_finished(true);
        return JArrow::Status::Finished;
    }

    if (in_status == SourceStatus::kSUCCESS && out_status == EventQueue::Status::Ready) {
        return JArrow::Status::KeepGoing;
    }

    return JArrow::Status::ComeBackLater;
}