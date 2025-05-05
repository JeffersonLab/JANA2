// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: Nathan Brei

#include "JANA/Utils/JEventLevel.h"
#include <JANA/Topology/JEventPool.h>


JEventPool::JEventPool(std::shared_ptr<JComponentManager> component_manager,
                       size_t max_inflight_events,
                       size_t location_count,
                       JEventLevel level)
        : JEventQueue(max_inflight_events, location_count)
        , m_component_manager(component_manager)
        , m_level(level) {

    // Create JEvents and distribute them among the pools.
    m_owned_events.reserve(max_inflight_events);
    for (size_t evt_idx=0; evt_idx<max_inflight_events; evt_idx++) {

        m_owned_events.push_back(std::make_shared<JEvent>());
        auto evt = &m_owned_events.back(); 
        (*evt)->SetLevel(m_level); // Level needs to be set before factories get added in configure_event
        m_component_manager->configure_event(**evt);
        Push(evt->get(), evt_idx % location_count);
    }
}

void JEventPool::Scale(size_t capacity) {
    auto old_capacity = m_capacity;
    if (capacity < old_capacity) {
        // Downscaling a JEventPool is tricky because even draining the topology
        // doesn't guarantee that all JEvents have been returned to the JEventPools by
        // the time the topology pauses. Consider JEventUnfolder, which may very well hold
        // on to a child event that it won't need because the parent event source has 
        // paused or finished.
        // For now we don't reduce the size of the pools or queues because there's little
        // penalty to keeping them around (the main penalty comes from event creation time 
        // and memory footprint, but if we are downscaling we already paid that)
        return;
    }

    // Resize queues to fit new capacity
    m_capacity = capacity;
    for (auto& local_queue: m_local_queues) {
        local_queue->ringbuffer.resize(capacity, nullptr);
        local_queue->capacity = capacity;
    }

    // Create new JEvents, add to owned_events, and distribute to queues
    m_owned_events.reserve(capacity);

    for (size_t evt_idx=old_capacity; evt_idx<capacity; evt_idx++) {
        m_owned_events.push_back(std::make_shared<JEvent>());
        auto evt = &m_owned_events.back(); 
        (*evt)->SetLevel(m_level); // Level needs to be set before factories get added in configure_event
        m_component_manager->configure_event(**evt);
        Push(evt->get(), evt_idx % GetLocationCount());
    }
}

void JEventPool::Finalize() {
    for (auto& evt : m_owned_events) {
        evt->Finish();
    }
}


