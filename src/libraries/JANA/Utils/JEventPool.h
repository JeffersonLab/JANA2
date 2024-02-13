
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTPOOL_H
#define JANA2_JEVENTPOOL_H

#include <JANA/JEvent.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Engine/JPool.h>


class JEventPool : public JPool<std::shared_ptr<JEvent>> {

    std::shared_ptr<JComponentManager> m_component_manager;
    JEventLevel m_level;

public:
    inline JEventPool(std::shared_ptr<JComponentManager> component_manager,
                      size_t pool_size,
                      size_t location_count,
                      bool limit_total_events_in_flight,
                      JEventLevel level = JEventLevel::Event)
        : JPool(pool_size, location_count, limit_total_events_in_flight)
        , m_component_manager(component_manager)
        , m_level(level) {
    }

    void configure_item(std::shared_ptr<JEvent>* item) override {
        (*item) = std::make_shared<JEvent>();
        (*item)->SetLevel(m_level);
        m_component_manager->configure_event(**item);

    }

    void release_item(std::shared_ptr<JEvent>* item) override {
        if (auto source = (*item)->GetJEventSource()) source->DoFinish(**item);
        (*item)->mFactorySet->Release();
        (*item)->mInspector.Reset();
        (*item)->GetJCallGraphRecorder()->Reset();
    }
};


#endif //JANA2_JEVENTPOOL_H
