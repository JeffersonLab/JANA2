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

#ifndef JANA2_JEVENTGROUPTRACKER_H
#define JANA2_JEVENTGROUPTRACKER_H

#include <JANA/Services/JServiceLocator.h>
#include <JANA/JObject.h>

#include <atomic>
#include <thread>

/// A persistent JObject
class JEventGroup : public JObject {

    const int m_group_id;
    std::atomic_int m_events_in_flight;
    std::atomic_bool m_group_closed;

    friend class JEventGroupManager;

    /// Construction of JEventGroup is restricted to JEventGroupManager. This enforces the
    /// invariant that pointer equality <=> group_id, assuming a singleton JEventGroupManager.
    explicit JEventGroup(int group_id) : m_group_id(group_id),
                                         m_events_in_flight(0),
                                         m_group_closed(true) {}

public:

    /// Record that another event belonging to this group has been emitted.
    /// This is meant to be called from JEventSource::GetEvent.
    void StartEvent() {
        m_events_in_flight += 1;
        m_group_closed = false;
    }

    /// Report an event as finished. If this was the last event in the group, IsGroupFinished will now return true.
    /// Please only call once per event, so that we don't have to maintain a set of outstanding event ids.
    /// This is meant to be called from JEventProcessor::Process.
    bool FinishEvent() {
        m_events_in_flight -= 1;
    }

    /// Indicate that no more events in the group are on their way. Note that groups can be re-opened
    /// by simply emitting another event tagged according to that group.
    /// This is meant to be called from JEventSource::GetEvent.
    void CloseGroup() {
        m_group_closed = true;
    }

    /// Test whether all events in the group have finished. Two conditions have to hold:
    /// 1. The number of in-flight events must be zero
    /// 2. The group must be closed. Otherwise, if the JEventSource is slow but the JEventProcessor is fast,
    ///    the number of in-flight events could drop to zero before the group is conceptually finished.
    /// This is meant to be callable from any JANA component.
    bool IsGroupFinished() {
        return m_group_closed && (m_events_in_flight == 0);
    }

    /// Block until every event in this group has finished, and the eventsource has declared the group closed.
    /// This is meant to be callable from any JANA component.
    void WaitUntilGroupFinished() {
        while (!(m_group_closed && (m_events_in_flight == 0))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

/// JEventGroupManager is a JService which
/// The purpose of JEventGroupManager is to
/// 1. Maintain ownership over all JEventGroup objects, ensuring that the pointers live the entire
///    duration of JApplication::Run() and are always deleted afterwards.
/// 2. Enforce the invariant where any two objects with the same identity (i.e. pointer equality) have
///    equal group ids. This makes debugging much easier.
/// 3. Encourage the practice of keeping state which is shared between different JEvents _explicit_ by using JServices.

class JEventGroupManager : public JService {

    std::map<int, JEventGroup*> m_eventgroups;

    JEventGroup* GetEventGroup(int group_id) {
        auto result = m_eventgroups.find(group_id);
        if (result == m_eventgroups.end()) {
            auto* eg = new JEventGroup(group_id);
            m_eventgroups.insert(std::make_pair(group_id, eg));
        }
        else {
            return result->second;
        }
    }

};


#endif //JANA2_JEVENTGROUPTRACKER_H
