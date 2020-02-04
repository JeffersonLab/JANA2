//
// Created by nbrei on 4/8/19.
//

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/Engine/JEventSourceArrow.h>
#include <JANA/Utils/JEventPool.h>


using SourceStatus = JEventSource::RETURN_STATUS;

JEventSourceArrow::JEventSourceArrow(std::string name,
                                     JAbstractEventSource* source,
                                     EventQueue* output_queue,
                                     std::shared_ptr<JEventPool> pool
                                     )
    : JArrow(name, false, NodeType::Source)
    , _source(source)
    , _output_queue(output_queue)
    , _pool(pool) {

    _output_queue->attach_upstream(this);
    attach_downstream(_output_queue);
    _logger = JLogger();
}



void JEventSourceArrow::execute(JArrowMetrics& result, size_t location_id) {

    if (!is_active()) {
        result.update_finished();
        return;
    }

    JAbstractEventSource::ReturnStatus in_status = JAbstractEventSource::ReturnStatus::Success;
    auto start_time = std::chrono::steady_clock::now();

    auto chunksize = get_chunksize();
    auto reserved_count = _output_queue->reserve(chunksize, location_id);
    auto emit_count = reserved_count;

    if (reserved_count != chunksize) {
        // Ensures that the source _only_ emits in increments of
        // chunksize, which happens to come in very handy for
        // processing entangled event blocks
        in_status = JAbstractEventSource::ReturnStatus::TryAgain;
        emit_count = 0;
        LOG_DEBUG(_logger) << "JEventSourceArrow asked for " << chunksize << ", but only reserved " << reserved_count << LOG_END;
    }
    else {
        auto event = _pool->get(location_id);
        for (size_t i=0; i<emit_count && in_status==JAbstractEventSource::ReturnStatus::Success; ++i) {
            if (event == nullptr) {
                in_status = JAbstractEventSource::ReturnStatus::TryAgain;
                break;
            }
            event->SetJEventSource(_source);
            event->SetJApplication(_source->GetApplication());
            in_status = _source->DoNext(event);
            if (in_status == JAbstractEventSource::ReturnStatus::Success) {
                _chunk_buffer.push_back(std::move(event));
                event = _pool->get(location_id);
            }
        }
    }

    auto latency_time = std::chrono::steady_clock::now();
    auto message_count = _chunk_buffer.size();
    auto out_status = _output_queue->push(_chunk_buffer, reserved_count, location_id);
    auto finished_time = std::chrono::steady_clock::now();

    LOG_DEBUG(_logger) << "JEventSourceArrow '" << get_name() << "' [" << location_id << "]: "
                       << "Emitted " << message_count << " events; last GetEvent "
                       << ((in_status==JAbstractEventSource::ReturnStatus::Success) ? "succeeded" : "failed")
                       << LOG_END;

    auto latency = latency_time - start_time;
    auto overhead = finished_time - latency_time;
    JArrowMetrics::Status status;

    if (in_status == JAbstractEventSource::ReturnStatus::Finished) {
        set_upstream_finished(true);
        LOG_DEBUG(_logger) << "JEventSourceArrow '" << get_name() << "': "
                           << "Finished!" << LOG_END;
        status = JArrowMetrics::Status::Finished;
    }
    else if (in_status == JEventSource::ReturnStatus::Success && out_status == EventQueue::Status::Ready) {
        status = JArrowMetrics::Status::KeepGoing;
    }
    else {
        status = JArrowMetrics::Status::ComeBackLater;
    }
    result.update(status, message_count, 1, latency, overhead);
}

void JEventSourceArrow::initialize() {
    assert(_status == Status::Unopened);
    LOG_DEBUG(_logger) << "JEventSourceArrow '" << get_name() << "': "
                      << "Initializing" << LOG_END;
    _source->DoInitialize();
    _status = Status::Running;
}

