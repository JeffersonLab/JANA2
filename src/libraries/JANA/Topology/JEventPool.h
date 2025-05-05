// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: Nathan Brei

#pragma once

#include <JANA/Topology/JEventQueue.h>
#include <JANA/Services/JComponentManager.h>
#include <vector>


class JEventPool : public JEventQueue {
private:
    std::vector<std::shared_ptr<JEvent>> m_owned_events;
    std::shared_ptr<JComponentManager> m_component_manager;
    JEventLevel m_level;
    JLogger m_logger;

public:
    inline JEventPool(std::shared_ptr<JComponentManager> component_manager,
                      size_t max_inflight_events,
                      size_t location_count,
                      JEventLevel level = JEventLevel::PhysicsEvent);

    void Scale(size_t capacity);

    void DetachAndPushParents(JEvent* event, size_t location);

    void Finalize();

};


