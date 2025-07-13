// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: Nathan Brei

#include "JANA/JEvent.h"
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
        m_component_manager->configure_event(**evt);
        (*evt)->SetLevel(m_level);
        Push(evt->get(), evt_idx % location_count);
    }
}

void JEventPool::AttachForwardingPool(JEventPool* pool) {
    m_parent_pools[pool->m_level] = pool;
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
        m_component_manager->configure_event(**evt);
        (*evt)->SetLevel(m_level);
        Push(evt->get(), evt_idx % GetLocationCount());
    }
}

void JEventPool::Ingest(JEvent* event, size_t location) {

    // Check if event even belongs here. If not, forward to the correct pool
    // This is necessary for interleaved events
    auto incoming_event_level = event->GetLevel();
    if (incoming_event_level != m_level) {
        LOG << "Pool " << toString(m_level) << " forwarding event " << event->GetEventNumber() << " to parent pool " << toString(event->GetLevel());
        m_parent_pools.at(incoming_event_level)->Ingest(event, location);
        return;
    }

    // Detach and foward parents
    auto finished_parents = event->ReleaseAllParents();
    // TODO: I'd prefer to not have to do an allocation each time, but this will work for now
    for (auto* parent : finished_parents) {
        LOG << "JEventPool::Ingest: Found finished parent of level " << toString(parent->GetLevel());
        m_parent_pools.at(parent->GetLevel())->NotifyThatAllChildrenFinished(parent, location);
        // TODO: This is likely the wrong location. Obtain from parent event?
    }

    if (event->GetChildCount() == 0) {
        // There's no way for additional children to appear because Ingest takes the "original" parent
        //LOG << "JEventPool::Ingest: event at level " << toString(m_level) << " is PUSHED";
        LOG << "JEventPool::Ingest: " << toString(m_level) << " event is pushed";
        Push(event, location);
    }
    else {
        // We've received the original but we can't push it until all children have been pushed to 
        // _their_ pools, in which case we push once we receive the notification
        LOG << "JEventPool::Ingest: " << toString(m_level) << " event is pending";
        m_pending.insert(event);
    }
}


void JEventPool::NotifyThatAllChildrenFinished(JEvent* event, size_t location) {
    LOG << "JEventPool::Notify called for level " << toString(m_level);
    size_t was_present = m_pending.erase(event);
    if (was_present == 1) {
        Push(event, location);
        LOG << "JEventPool at level " << toString(m_level) << " has pushed a parent event";
    }
}


void JEventPool::Finalize() {
    for (auto& evt : m_owned_events) {
        evt->Finish();
    }
}


