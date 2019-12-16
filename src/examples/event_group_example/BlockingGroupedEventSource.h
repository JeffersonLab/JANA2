//
// Created by Nathan Brei on 2019-12-15.
//

#ifndef JANA2_TRIDASEVENTSOURCE_H
#define JANA2_TRIDASEVENTSOURCE_H

#include <JANA/JEventSource.h>
#include <JANA/Services/JEventGroupTracker.h>

#include "TridasEvent.h"

#include <queue>

class BlockingGroupedEventSource : public JEventSource {

    JEventGroupManager m_egm;
    JEventGroup* m_current_group;
    int m_current_group_id;

    std::mutex m_pending_mutex;
    std::queue<TridasEvent*> m_pending_events;

public:

    BlockingGroupedEventSource(std::string res_name, JApplication* app) : JEventSource(std::move(res_name), app) {
        // TODO: Get EventGroupManager from ServiceLocator instead
        m_current_group_id = -1;
        m_current_group = nullptr;
    };


    /// SubmitAndWait provides a blocking interface for pushing groups of TridasEvents into JANA.
    /// JANA does NOT assume ownership of the events vector, nor does it clear it.
    void SubmitAndWait(int group_id, std::vector<TridasEvent*>& events) {
        auto group = m_egm.GetEventGroup(group_id);
        {
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            for (auto event : events) {
                event->group_number = group_id;
                group->StartEvent();   // We have to call this immediately in order to 'open' the group
                m_pending_events.push(event);
            }
        }
        group->CloseGroup();
        group->WaitUntilGroupFinished();
    }


    /// GetEvent polls the queue of submitted TridasEvents and feeds them into JEvents along with a
    /// JEventGroup. A downstream EventProcessor may report the event as being finished. Once all
    /// events in the eventgroup are finished, the corresponding call to SubmitAndWait will unblock.
    void GetEvent(std::shared_ptr<JEvent> event) override {

        // Retrieve the next Tridas event from the queue
        TridasEvent* tridas_event = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            if (m_pending_events.empty()) {
                throw RETURN_STATUS::kTRY_AGAIN;
            }

            tridas_event = m_pending_events.front();
            m_pending_events.pop();
        }

        // Associate it with the correct group object
        if (m_current_group_id != tridas_event->group_number || m_current_group == nullptr) {
            m_current_group_id = tridas_event->group_number;
            m_current_group = m_egm.GetEventGroup(m_current_group_id);
        }

        // Hydrate JEvent with both the TridasEvent and the group pointer.
        event->Insert(tridas_event);
        event->Insert(m_current_group);

        // Tell JANA not to assume ownership of these objects!
        event->GetFactory<TridasEvent>()->SetFactoryFlag(JFactory::JFactory_Flags_t::NOT_OBJECT_OWNER);
        event->GetFactory<JEventGroup>()->SetFactoryFlag(JFactory::JFactory_Flags_t::NOT_OBJECT_OWNER);

        // JANA always needs an event number and a run number, so extract these from the Tridas data somehow
        event->SetEventNumber(tridas_event->event_number);
        event->SetRunNumber(tridas_event->run_number);
    }
};


#endif //JANA2_TRIDASEVENTSOURCE_H
