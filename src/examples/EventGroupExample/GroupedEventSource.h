//
// Created by Nathan Brei on 2019-12-15.
//

#ifndef JANA2_GROUPEDEVENTSOURCE_H
#define JANA2_GROUPEDEVENTSOURCE_H

#include <JANA/JEventSource.h>
#include <JANA/Services/JEventGroupTracker.h>

class GroupedEventSource : public JEventSource {

    JEventGroupManager m_egm;
    JEventGroup* m_current_group;
    int m_remaining_events_in_group;
    int m_current_group_id;
    int m_current_event_number;

public:
    GroupedEventSource(std::string res_name, JApplication* app) : JEventSource(std::move(res_name), app) {
        // TODO: Get EventGroupManager from ServiceLocator instead
        m_remaining_events_in_group = 5;
        m_current_group_id = 0;
        m_current_event_number = 0;
    };

    void GetEvent(std::shared_ptr<JEvent> event) override {

        if (m_current_group_id == 5) {
            throw RETURN_STATUS::kNO_MORE_EVENTS;
        }

        // TODO: We can hold on to the pointer instead of doing the lookup everytime
        auto current_group = m_egm.GetEventGroup(m_current_group_id);

        current_group->StartEvent();
        event->Insert(current_group);
        event->GetFactory<JEventGroup>()->SetFactoryFlag(JFactory::JFactory_Flags_t::NOT_OBJECT_OWNER);
        event->SetEventNumber(++m_current_event_number);
        event->SetRunNumber(m_current_group_id);

        m_remaining_events_in_group -= 1;
        if (m_remaining_events_in_group == 0) {
            current_group->CloseGroup();
            m_remaining_events_in_group = 5;
            m_current_group_id += 1;
        }

    }
};


#endif //JANA2_GROUPEDEVENTSOURCE_H
