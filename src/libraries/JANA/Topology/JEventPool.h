// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: Nathan Brei

#pragma once

#include <JANA/Topology/JEventQueue.h>
#include <JANA/Services/JComponentManager.h>
#include <unordered_set>
#include <vector>


class JEventPool : public JEventQueue {
private:
    std::map<JEventLevel, JEventPool*> m_parent_pools;
    std::unordered_set<JEvent*> m_pending;

    std::vector<std::shared_ptr<JEvent>> m_owned_events;
    std::shared_ptr<JComponentManager> m_component_manager;
    JEventLevel m_level;
    JLogger m_logger;

public:
    JEventPool(std::shared_ptr<JComponentManager> component_manager,
               size_t max_inflight_events,
               size_t location_count,
               JEventLevel level = JEventLevel::PhysicsEvent);

    void Scale(size_t capacity);

    void Ingest(JEvent* event, size_t location);

    void NotifyThatAllChildrenFinished(JEvent* event, size_t location);

    void Finalize();

};


