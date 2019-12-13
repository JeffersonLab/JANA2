//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//


#include <JANA/JEventProcessor.h>
#include <JANA/Services/JEventGroupTracker.h>
#include <JANA/JFactory.h>
#include <JANA/Utils/JPerfUtils.h>

class JEventSource_eventgroups : public JEventSource {

    JEventGroupManager m_egm;
    JEventGroup* m_current_group;
    int m_remaining_events_in_group;
    int m_current_group_id;
    int m_current_event_number;

public:
    JEventSource_eventgroups(std::string res_name, JApplication* app) : JEventSource(std::move(res_name), app) {
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



class JEventProcessor_eventgroups : public JEventProcessor {
    std::mutex m_mutex;

public:
    void Process(const std::shared_ptr<const JEvent>& event) override {
        // In parallel, perform a random amount of (slow) computation
        consume_cpu_ms(100, 1.0);

        // Sequentially, process each event and report when a group finishes
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "Processing event "
                  << event->GetRunNumber()
                  << ":" << event->GetEventNumber() << std::endl;

        auto group = event->GetSingle<JEventGroup>();
        bool finishes_group = group->FinishEvent();

        if (finishes_group) {
            std::cout << "Finishing group " << group->GetGroupId() << std::endl;
        }
    }
};


extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);
    app->Add(new JEventSource_eventgroups("dummy_source", app));
    app->Add(new JEventProcessor_eventgroups());

}
} // "C"
