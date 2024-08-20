
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/Topology/JEventSourceArrow.h>
#include <JANA/Utils/JEventPool.h>



JEventSourceArrow::JEventSourceArrow(std::string name,
                                     std::vector<JEventSource*> sources)
    : JPipelineArrow(name, false, true, false), m_sources(sources) {
}


void JEventSourceArrow::process(Event* event, bool& success, JArrowMetrics::Status& arrow_status) {

    // If there are no sources available then we are automatically finished.
    if (m_sources.empty()) {
        success = false;
        arrow_status = JArrowMetrics::Status::Finished;
        return;
    }

    while (m_current_source < m_sources.size()) {

        LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << LOG_END;
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
            return;
        }
        else {
            LOG_DEBUG(m_logger) << "Executed arrow " << get_name() << " with result Success, emitting event# " << (*event)->GetEventNumber() << LOG_END;
            success = true;
            arrow_status = JArrowMetrics::Status::KeepGoing;
            return;
        }
    }
    success = false;
    arrow_status = JArrowMetrics::Status::Finished;
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
