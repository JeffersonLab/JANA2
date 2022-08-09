
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTGROUPTRACKER_H
#define JANA2_JEVENTGROUPTRACKER_H

#include <JANA/Services/JServiceLocator.h>
#include <JANA/JObject.h>

#include <atomic>
#include <thread>

/// A persistent JObject
class JEventGroup : public JObject {

    const int m_group_id;
    mutable std::atomic_int m_events_in_flight;
    mutable std::atomic_bool m_group_closed;

    friend class JEventGroupManager;

    /// Construction of JEventGroup is restricted to JEventGroupManager. This enforces the
    /// invariant that pointer equality <=> group_id, assuming a singleton JEventGroupManager.
    explicit JEventGroup(int group_id) : m_group_id(group_id),
                                         m_events_in_flight(0),
                                         m_group_closed(true) {}

public:

    /// Report back what group this actual is. This is mostly for debugging purposes.
    int GetGroupId() const {
        return m_group_id;
    }

    /// Record that another event belonging to this group has been emitted.
    /// This is meant to be called from JEventSource::GetEvent.
    void StartEvent() const {
        m_events_in_flight += 1;
        m_group_closed = false;
    }

    /// Report an event as finished. If this was the last event in the group, IsGroupFinished will now return true.
    /// Please only call once per event, so that we don't have to maintain a set of outstanding event ids.
    /// This takes advantage of C++ atomics to detect if _we_ were the one who finished the whole group without
    /// needing a lock.
    /// This is meant to be called from JEventProcessor::Process.
    bool FinishEvent() const {
        auto prev_events_in_flight = m_events_in_flight.fetch_sub(1);
        assert(prev_events_in_flight > 0); // detect if someone is miscounting
        return (prev_events_in_flight == 1) && m_group_closed;
    }

    /// Indicate that no more events in the group are on their way. Note that groups can be re-opened
    /// by simply emitting another event tagged according to that group.
    /// This is meant to be called from JEventSource::GetEvent.
    void CloseGroup() const {
        m_group_closed = true;
    }

    /// Test whether all events in the group have finished. Two conditions have to hold:
    /// 1. The number of in-flight events must be zero
    /// 2. The group must be closed. Otherwise, if the JEventSource is slow but the JEventProcessor is fast,
    ///    the number of in-flight events could drop to zero before the group is conceptually finished.
    /// This is meant to be callable from any JANA component.
    /// Note that this doesn't indicate anything about _who_
    bool IsGroupFinished() const {
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

class JEventGroupManager final : public JService {

    std::mutex m_mutex;
    std::map<int, JEventGroup*> m_eventgroups;

public:
    ~JEventGroupManager() final {
        for (auto item : m_eventgroups) {
            delete item.second;
        }
    }

    JEventGroup* GetEventGroup(int group_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto result = m_eventgroups.find(group_id);
        if (result == m_eventgroups.end()) {
            auto* eg = new JEventGroup(group_id);
            m_eventgroups.insert(std::make_pair(group_id, eg));
            return eg;
        }
        else {
            return result->second;
        }
    }

};


#endif //JANA2_JEVENTGROUPTRACKER_H
