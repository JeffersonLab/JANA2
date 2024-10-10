
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Utils/JEventPool.h>



JEventSourceArrow::JEventSourceArrow(std::string name, std::vector<JEventSource*> sources)
    : JArrow(name, false, true, false), m_sources(sources) {
}


void JEventSourceArrow::execute(JArrowMetrics& result, size_t location_id) {

    auto start_total_time = std::chrono::steady_clock::now();

    Data<Event> in_data {location_id};
    Data<Event> out_data {location_id};

    bool success = m_input.pull(in_data) && m_output.pull(out_data);
    if (!success) {
        m_input.revert(in_data);
        m_output.revert(out_data);
        // TODO: Test that revert works properly
        
        auto end_total_time = std::chrono::steady_clock::now();
        result.update(JArrowMetrics::Status::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
        return;
    }

    bool process_succeeded = true;
    JArrowMetrics::Status process_status = JArrowMetrics::Status::KeepGoing;

    // If we have a pending barrier event, the input event will just be nullptr
    Event* event_in = (in_data.item_count == 1) ? in_data.items[0] : nullptr;

    auto start_processing_time = std::chrono::steady_clock::now();
    Event* event_out = process(event_in, process_succeeded, process_status);
    auto end_processing_time = std::chrono::steady_clock::now();

    if (process_succeeded) {
        if (event_out == nullptr) {
            // Event will be nullptr if the JEventSource emitted a barrier event that must 
            // be held in m_pending_barrier_event until all preceding events have finished
            in_data.item_count = 0; // Nothing gets returned to the input queue
            out_data.item_count = 0; // Nothing gets sent to the output queue
            m_input.min_item_count = 0; // We don't ask for any events from the input queue next time
            m_input.max_item_count = 0;
        }
        else {
            in_data.item_count = 0; // Nothing gets returned to the input queue
            out_data.item_count = 1; // Event gets sent to the output queue
            out_data.items[0] = event_out;
            m_input.min_item_count = 1; // We ask for a fresh event from the input queue next time
            m_input.max_item_count = 1;
        }
    }
    m_input.push(in_data);
    m_output.push(out_data);

    // Publish metrics
    auto end_total_time = std::chrono::steady_clock::now();
    auto latency = (end_processing_time - start_processing_time);
    auto overhead = (end_total_time - start_total_time) - latency;
    result.update(process_status, process_succeeded, 1, latency, overhead);
}


Event* JEventSourceArrow::process(Event* event, bool& success, JArrowMetrics::Status& arrow_status) {

    LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << LOG_END;

    // First check to see if we need to handle a barrier event before attempting to emit another event
    if (m_barrier_active) {
        // A barrier event has been emitted by the source.
        if (m_pending_barrier_event != nullptr) {
            // This barrier event is pending until the topology drains
            if (m_sources[m_current_source]->GetEmittedEventCount() - 
                m_sources[m_current_source]->GetFinishedEventCount() == 1) {
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Barrier event is in-flight" << LOG_END;

                // Topology has drained; only remaining in-flight event is the barrier event itself,
                // which we have held on to until now
                Event* barrier_event = m_pending_barrier_event;
                m_pending_barrier_event = nullptr;
                return barrier_event;
            }
            else {
                // Topology has _not_ finished draining, all we can do is wait
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Waiting on pending barrier event" << LOG_END;
                LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result ComeBackLater"<< LOG_END;
                arrow_status = JArrowMetrics::Status::ComeBackLater;
                success = false;
                return nullptr;
            }
        }
        else {
            // This barrier event has already been sent into the topology and we need to wait
            // until it is finished before emitting any more events
            if (m_sources[m_current_source]->GetFinishedEventCount() == 
                m_sources[m_current_source]->GetEmittedEventCount()) {
                
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Barrier event finished, returning to normal operation" << LOG_END;

                // Barrier event has finished
                m_barrier_active = false;
                // Continue to emit the next event
            }
            else {
                // Barrier event has NOT finished
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Waiting on in-flight barrier event" << LOG_END;
                LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result ComeBackLater"<< LOG_END;
                success = false;
                arrow_status = JArrowMetrics::Status::ComeBackLater;
                return nullptr;
            }
        }
    }

    while (m_current_source < m_sources.size()) {

        auto source_status = m_sources[m_current_source]->DoNext(*event);

        if (source_status == JEventSource::Result::FailureFinished) {
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result FailureFinished"<< LOG_END;
            m_current_source++;
            // TODO: Adjust nskip and nevents for the new source
        }
        else if (source_status == JEventSource::Result::FailureTryAgain){
            // This JEventSource isn't finished yet, so we obtained either Success or TryAgainLater
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result FailureTryAgain"<< LOG_END;
            success = false;
            arrow_status = JArrowMetrics::Status::ComeBackLater;
            return event;
        }
        else if ((*event)->GetSequential()){
            // Source succeeded, but returned a barrier event
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result Success, holding back barrier event# " << (*event)->GetEventNumber() << LOG_END;
            m_pending_barrier_event = event;
            m_barrier_active = true;
            return nullptr;
        }
        else {
            // Source succeeded, did NOT return a barrier event
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result Success, emitting event# " << (*event)->GetEventNumber() << LOG_END;
            success = true;
            arrow_status = JArrowMetrics::Status::KeepGoing;
            return event;
        }
    }
    success = false;
    arrow_status = JArrowMetrics::Status::Finished;
    return event;
}

void JEventSourceArrow::initialize() {
    // We initialize everything immediately, but don't open any resources until we absolutely have to; see process(): source->DoNext()
    for (JEventSource* source : m_sources) {
        source->DoInit();
    }
}

void JEventSourceArrow::finalize() {
    // Generally JEventSources finalize themselves as soon as they detect that they have run out of events.
    // However, we can't rely on the JEventSources turning themselves off since execution can be externally paused.
    // Instead we leave everything open until we finalize the whole topology, and finalize remaining event sources then.
    for (JEventSource* source : m_sources) {
        source->DoClose();
    }
}
