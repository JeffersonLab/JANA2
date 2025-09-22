
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/Topology/JEventSourceArrow.h>



JEventSourceArrow::JEventSourceArrow(std::string name, std::vector<JEventSource*> sources)
    : m_sources(sources) {
    set_name(name);
    set_is_source(true);
    create_ports(1, 1);
    m_ports[EVENT_OUT].establishes_ordering = true;
    // All event sources establish their own ordering by default,
    // which is sufficient for the kinds of topologies we can create
    // using JTopologyBuilder. In the future we may need something
    // more sophisticated. Note that establishing the ordering here is a
    // no-op if there are no components in the topology that need it enforced,
    // so it doesn't hurt NUMA performance.
}


void JEventSourceArrow::fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) {

    LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << LOG_END;

    // First check to see if we need to handle a barrier event before attempting to emit another event
    if (m_barrier_active) {

        auto emitted_event_count = m_sources[m_current_source]->GetEmittedEventCount();
        auto finished_event_count = m_sources[m_current_source]->GetProcessedEventCount();

        // A barrier event has been emitted by the source.
        if (m_pending_barrier_event != nullptr) {

            // This barrier event is pending until the topology drains
            if ((emitted_event_count - finished_event_count) == 1) {
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Barrier event is in-flight" << LOG_END;

                // Topology has drained; only remaining in-flight event is the barrier event itself,
                // which we have held on to until now
                outputs[0] = {m_pending_barrier_event, 1};
                output_count = 1;
                status = JArrow::FireResult::KeepGoing;

                m_pending_barrier_event = nullptr;
                // There's not much for the thread team to do while the barrier event makes its way through the topology.
                // Eventually we might be able to use this to communicate to the scheduler to not wake threads whose only
                // available action is to hammer the JEventSourceArrow
                return;
            }
            else {
                // Topology has _not_ finished draining, all we can do is wait
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Waiting on pending barrier event. Emitted = " << emitted_event_count << ", Finished = " << finished_event_count << LOG_END;
                LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result ComeBackLater"<< LOG_END;

                assert(event == nullptr);
                output_count = 0;
                status = JArrow::FireResult::ComeBackLater;
                return;
            }
        }
        else {
            // This barrier event has already been sent into the topology and we need to wait
            // until it is finished before emitting any more events
            if (finished_event_count == emitted_event_count) {

                // Barrier event has finished.
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Barrier event finished, returning to normal operation" << LOG_END;
                m_barrier_active = false;
                m_next_input_port = 0;

                output_count = 0;
                status = JArrow::FireResult::KeepGoing;
                return;
            }
            else {
                // Barrier event has NOT finished
                LOG_DEBUG(m_logger) << "JEventSourceArrow: Waiting on in-flight barrier event" << LOG_END;
                LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result ComeBackLater"<< LOG_END;
                assert(event == nullptr);
                output_count = 0;
                status = JArrow::FireResult::ComeBackLater;
                return;
            }
        }
    }

    while (m_current_source < m_sources.size()) {

        auto source_status = m_sources[m_current_source]->DoNext(event->shared_from_this());

        if (source_status == JEventSource::Result::FailureFinished) {
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result FailureFinished"<< LOG_END;
            m_current_source++;
            // TODO: Adjust nskip and nevents for the new source
        }
        else if (source_status == JEventSource::Result::FailureTryAgain){
            // This JEventSource isn't finished yet, so we obtained either Success or TryAgainLater
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result ComeBackLater"<< LOG_END;
            outputs[0] = {event, 0}; // Reject
            output_count = 1;
            status = JArrow::FireResult::ComeBackLater;
            return;
        }
        else if (event->GetSequential()){
            // Source succeeded, but returned a barrier event
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result Success, holding back barrier event# " << event->GetEventNumber() << LOG_END;
            m_pending_barrier_event = event;
            m_barrier_active = true;
            m_next_input_port = -1; // Stop popping events from the input queue until barrier event has finished
            
            // Arrow hangs on to the barrier event until the topology fully drains
            output_count = 0;
            status = JArrow::FireResult::KeepGoing; // Mysteriously livelocks if we set this to ComeBackLater??
            return;
        }
        else {
            // Source succeeded, did NOT return a barrier event
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result Success, emitting event# " << event->GetEventNumber() << LOG_END;
            outputs[0] = {event, 1}; // SUCCESS!
            output_count = 1;
            status = JArrow::FireResult::KeepGoing;
            return;
        }
    }

    // All event sources have finished now
    outputs[0] = {event, 0}; // Reject event
    output_count = 1;
    status = JArrow::FireResult::Finished;
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
