
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.



#ifndef JANA2_GROUPEDEVENTSOURCE_H
#define JANA2_GROUPEDEVENTSOURCE_H

#include <JANA/JEventSource.h>
#include <JANA/Services/JEventGroupTracker.h>

class GroupedEventSource : public JEventSource {

    JEventGroupManager m_egm;
    // JEventGroup* m_current_group;
    int m_remaining_events_in_group;
    int m_current_group_id;
    int m_current_event_number;

public:
    GroupedEventSource(std::string res_name, JApplication* app) : JEventSource(std::move(res_name), app) {
        // TODO: Get EventGroupManager from ServiceLocator instead
        SetCallbackStyle(CallbackStyle::ExpertMode);
        m_remaining_events_in_group = 5;
        m_current_group_id = 0;
        m_current_event_number = 0;
    };

    Result Emit(JEvent& event) override {

        if (m_current_group_id == 5) {
            return Result::FailureFinished;
        }

        // TODO: We can hold on to the pointer instead of doing the lookup everytime
        auto current_group = m_egm.GetEventGroup(m_current_group_id);

        current_group->StartEvent();
        event.Insert(current_group);
        event.GetFactory<JEventGroup>()->SetFactoryFlag(JFactory::JFactory_Flags_t::NOT_OBJECT_OWNER);
        event.SetEventNumber(++m_current_event_number);
        event.SetRunNumber(m_current_group_id);

        m_remaining_events_in_group -= 1;
        if (m_remaining_events_in_group == 0) {
            current_group->CloseGroup();
            m_remaining_events_in_group = 5;
            m_current_group_id += 1;
        }

        return Result::Success;
    }
};


#endif //JANA2_GROUPEDEVENTSOURCE_H
