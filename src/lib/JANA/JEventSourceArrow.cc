//
// Created by nbrei on 4/8/19.
//

#include <greenfield/Components.h>
#include <JANA/JEventSourceArrow.h>

using JEventSourceStatus = JEventSource::RETURN_STATUS;
using SourceStatus = greenfield::Source<const Event>::Status;
using ArrowStatus = JEventSourceArrow::Status;

JEventSourceArrow::JEventSourceArrow(std::string name, JEventSource& source, EventQueue* output_queue)
    : Arrow(name, false)
    , _source(source)
    , _output_queue(output_queue) {

    _output_queue->attach_upstream(this);
    attach_downstream(_output_queue);
}

Arrow::Status JEventSourceArrow::execute() {

    if (!is_active()) {
        return Arrow::Status::Finished;
    }
    if (!_is_initialized) {
        _source.Open();
        _is_initialized = true;
    }

    SourceStatus in_status = SourceStatus::KeepGoing;
    auto start_time = std::chrono::steady_clock::now();
    try {
        size_t item_count = get_chunksize();
        while (item_count-- != 0) {
            _chunk_buffer.push_back(_source.GetEvent());
        }
    }
    catch (JEventSource::RETURN_STATUS rs) {
        switch (rs) {
            case JEventSourceStatus::kSUCCESS:        in_status = SourceStatus::KeepGoing; break;
            case JEventSourceStatus::kNO_MORE_EVENTS: in_status = SourceStatus::Finished; break;
            case JEventSourceStatus::kTRY_AGAIN:
            case JEventSourceStatus::kBUSY:           in_status = SourceStatus::ComeBackLater; break;
            case JEventSourceStatus::kERROR:
            case JEventSourceStatus::kUNKNOWN:        in_status = SourceStatus::Error; break;
        }
    }
    catch (...) {
        in_status = SourceStatus::Error;
    }
    auto latency_time = std::chrono::steady_clock::now();
    auto message_count = _chunk_buffer.size();
    auto out_status = _output_queue->push(_chunk_buffer);
    _chunk_buffer.clear();
    auto finished_time = std::chrono::steady_clock::now();

    auto latency = latency_time - start_time;
    auto overhead = finished_time - latency_time;
    update_metrics(message_count, 1, latency, overhead);

    if (in_status == SourceStatus::Finished) {
        // There should be a _source.Close() of some kind
        set_upstream_finished(true);
        return Arrow::Status::Finished;
    }

    if (in_status == SourceStatus::KeepGoing && out_status == EventQueue::Status::Ready) {
        return Arrow::Status::KeepGoing;
    }

    return Arrow::Status::ComeBackLater;
}