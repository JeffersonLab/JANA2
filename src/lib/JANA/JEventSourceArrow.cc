//
// Created by nbrei on 4/8/19.
//

#include <JANA/JEventSourceArrow.h>
#include <JANA/JApplication.h>
#include <JANA/JEventPool.h>


using SourceStatus = JEventSource::RETURN_STATUS;

JEventSourceArrow::JEventSourceArrow(std::string name,
                                     JEventSource* source,
                                     EventQueue* output_queue,
                                     std::shared_ptr<JEventPool> pool
                                     )
    : JArrow(name, false, NodeType::Source)
    , _source(source)
    , _output_queue(output_queue)
    , _pool(pool) {

    _output_queue->attach_upstream(this);
    attach_downstream(_output_queue);
    _logger = JLogger::nothing();
}



void JEventSourceArrow::execute(JArrowMetrics& result, size_t location_id) {

    if (!is_active()) {
        result.update_finished();
        return;
    }

    SourceStatus in_status = SourceStatus::kSUCCESS;
    auto start_time = std::chrono::steady_clock::now();
    try {
        size_t item_count = get_chunksize();
        while (item_count-- != 0) {
            auto event = _pool->get(location_id);
            event->SetJEventSource(_source);
            _source->GetEvent(event);
            _chunk_buffer.push_back(std::move(event));
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
    auto out_status = _output_queue->push(_chunk_buffer, location_id);
    _chunk_buffer.clear();
    auto finished_time = std::chrono::steady_clock::now();

    LOG_DEBUG(_logger) << "JEventSourceArrow '" << get_name() << "' [" << location_id << "]: "
                       << "Emitted " << message_count << " events; last GetEvent "
                       << ((in_status==SourceStatus::kSUCCESS) ? "succeeded" : "failed")
                       << LOG_END;

    auto latency = latency_time - start_time;
    auto overhead = finished_time - latency_time;
    JArrowMetrics::Status status;

    if (in_status == SourceStatus::kNO_MORE_EVENTS) {
        // There should be a _source.Close() of some kind
        set_upstream_finished(true);
        LOG_DEBUG(_logger) << "JEventSourceArrow '" << get_name() << "': "
                           << "Finished!" << LOG_END;
        status = JArrowMetrics::Status::Finished;
    }
    else if (in_status == SourceStatus::kSUCCESS && out_status == EventQueue::Status::Ready) {
        status = JArrowMetrics::Status::KeepGoing;
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
    }
    result.update(status, message_count, 1, latency, overhead);
}

void JEventSourceArrow::initialize() {
    assert(_status == Status::Unopened);
    LOG_INFO(_logger) << "JEventSourceArrow '" << get_name() << "': "
                      << "Initializing" << LOG_END;
    _source->Open();
    _status = Status::Inactive;
}

