//
// Created by Nathan Brei on 4/30/20.
//

#include "ComponentArrows.h"
#include <JANA/JEventProcessor.h>


namespace jana {
namespace arrowengine {

using Status = EventSourceOp::Status;

std::pair<Status, Event> EventSourceOp::next() {

    auto event = m_pool->get(0);
    if (event == nullptr) {
        return {Status::FailTryAgain, nullptr};
    }
    event->SetJEventSource(m_source);
    event->SetJApplication(m_source->GetApplication());
    auto in_status = m_source->DoNext(event);
    if (in_status == JEventSource::ReturnStatus::Success) {
        return {Status::Success, event};
    }
    else if (in_status == JEventSource::ReturnStatus::Finished) {
        m_pool->put(event, 0);
        return {Status::FailFinished, nullptr};
    }
    else {
        m_pool->put(event, 0);
        return {Status::FailTryAgain, nullptr};
    }
}


Event EventProcessorOp::map(Event e) {
    for (JEventProcessor* processor : m_processors) {
        processor->DoMap(e);
    }
    return e;
}


}
}
