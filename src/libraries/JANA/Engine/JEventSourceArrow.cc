
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/Engine/JEventSourceArrow.h>
#include <JANA/Utils/JEventPool.h>


using SourceStatus = JEventSource::RETURN_STATUS;

JEventSourceArrow::JEventSourceArrow(std::string name,
                                     std::vector<JEventSource*> sources,
                                     EventQueue* output_queue,
                                     std::shared_ptr<JEventPool> pool
                                     )
    : JArrow(name, false, NodeType::Source)
    , m_sources(sources)
    , m_output_queue(output_queue)
    , m_pool(pool) {
}



void JEventSourceArrow::execute(JArrowMetrics& result, size_t location_id) {

    JEventSource::ReturnStatus in_status = JEventSource::ReturnStatus::Success;
    auto start_time = std::chrono::steady_clock::now();

    auto chunksize = get_chunksize();
    auto reserved_count = m_output_queue->reserve(chunksize, location_id);
    auto emit_count = reserved_count;

    if (reserved_count != chunksize) {
        // Ensures that the source _only_ emits in increments of
        // chunksize, which happens to come in very handy for
        // processing entangled event blocks
        in_status = JEventSource::ReturnStatus::TryAgain;
        emit_count = 0;
        LOG_DEBUG(m_logger) << "JEventSourceArrow asked for " << chunksize << ", but only reserved " << reserved_count << LOG_END;
    }
    else {
        for (size_t i=0; i<emit_count && in_status==JEventSource::ReturnStatus::Success; ++i) {
            auto event = m_pool->get(location_id);
            if (event == nullptr) {
                in_status = JEventSource::ReturnStatus::TryAgain;
                break;
            }
            while (m_current_source < m_sources.size()) {
                in_status = m_sources[m_current_source]->DoNext(event);

                if (in_status == JEventSource::ReturnStatus::Finished) {
                    m_current_source++;
                    // TODO: Adjust nskip and nevents for the new source
                }
                else {
                    // This JEventSource isn't finished yet, so we obtained either Success or TryAgainLater
                    break;
                }
            }
            if (in_status == JEventSource::ReturnStatus::Success) {
                m_chunk_buffer.push_back(std::move(event));
            }
            else {
                m_pool->put(event, location_id);
            }
        }
    }

    auto latency_time = std::chrono::steady_clock::now();
    auto message_count = m_chunk_buffer.size();
    auto out_status = m_output_queue->push(m_chunk_buffer, reserved_count, location_id);
    auto finished_time = std::chrono::steady_clock::now();

    if (message_count != 0) {
        LOG_DEBUG(m_logger) << "JEventSourceArrow '" << get_name() << "' [" << location_id << "]: "
                            << "Emitted " << message_count << " events; last GetEvent "
                            << ((in_status==JEventSource::ReturnStatus::Success) ? "succeeded" : "failed")
                            << LOG_END;
    }
    else {
        LOG_TRACE(m_logger) << "JEventSourceArrow emitted nothing" << LOG_END;
    }

    auto latency = latency_time - start_time;
    auto overhead = finished_time - latency_time;
    JArrowMetrics::Status status;

    if (in_status == JEventSource::ReturnStatus::Finished) {
        finish();
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
    // Initialization of individual sources happens on-demand, in order to keep us from having lots of open files
}

void JEventSourceArrow::finalize() {
    // Generally JEventSources finalize themselves as soon as they detect that they have run out of events.
    // However, we can't rely on the JEventSources turning themselves off since execution can be externally paused.
    // Instead we leave everything open until we finalize the whole topology, and finalize remaining event sources then.
    for (JEventSource* source : m_sources) {
        if (source->GetStatus() == JEventSource::SourceStatus::Opened) {
            LOG_INFO(m_logger) << "Finalizing JEventSource '" << source->GetTypeName() << "' (" << source->GetResourceName() << ")" << LOG_END;
            source->DoFinalize();
        }
    }
}
