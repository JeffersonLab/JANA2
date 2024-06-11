
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.



#ifndef JANA2_TRIDASEVENTSOURCE_H
#define JANA2_TRIDASEVENTSOURCE_H

#include <JANA/JEventSource.h>
#include <JANA/Services/JEventGroupTracker.h>

#include "TridasEvent.h"

#include <queue>

class BlockingGroupedEventSource : public JEventSource {

    JEventGroupManager m_egm;

    int m_pending_group_id;
    std::mutex m_pending_mutex;
    std::queue<std::pair<TridasEvent*, JEventGroup*>> m_pending_events;

public:

    BlockingGroupedEventSource(std::string res_name, JApplication* app) : JEventSource(std::move(res_name), app) {
        // TODO: Get EventGroupManager from ServiceLocator instead
        SetCallbackStyle(CallbackStyle::ExpertMode);
        m_pending_group_id = 1;
    };


    /// SubmitAndWait provides a blocking interface for pushing groups of TridasEvents into JANA.
    /// JANA does NOT assume ownership of the events vector, nor does it clear it.
    void SubmitAndWait(std::vector<TridasEvent*>& events) {
        auto group = m_egm.GetEventGroup(m_pending_group_id++);
        {
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            for (auto event : events) {
                group->StartEvent();   // We have to call this immediately in order to 'open' the group
                m_pending_events.push(std::make_pair(event, group));
            }
        }
        group->CloseGroup();
        group->WaitUntilGroupFinished();
    }


    /// GetEvent polls the queue of submitted TridasEvents and feeds them into JEvents along with a
    /// JEventGroup. A downstream EventProcessor may report the event as being finished. Once all
    /// events in the eventgroup are finished, the corresponding call to SubmitAndWait will unblock.
    Result Emit(JEvent& event) override {

        std::pair<TridasEvent*, JEventGroup*> next_event;
        {
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            if (m_pending_events.empty()) {
                return Result::FailureTryAgain;
            }
            else {
                next_event = m_pending_events.front();
                m_pending_events.pop();
            }
        }

        // Hydrate JEvent with both the TridasEvent and the group pointer.
        event.Insert(next_event.first);  // TridasEvent
        event.Insert(next_event.second); // JEventGroup

        // Tell JANA not to assume ownership of these objects!
        event.GetFactory<TridasEvent>()->SetNotOwnerFlag(true);
        event.GetFactory<JEventGroup>()->SetNotOwnerFlag(true);

        // JANA always needs an event number and a run number, so extract these from the Tridas data somehow
        event.SetEventNumber(next_event.first->event_number);
        event.SetRunNumber(next_event.first->run_number);

        return Result::Success;
    }
};


#endif //JANA2_TRIDASEVENTSOURCE_H
