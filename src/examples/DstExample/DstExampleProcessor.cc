
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "DstExampleProcessor.h"
#include "DataObjects.h"
#include <JANA/JLogger.h>

DstExampleProcessor::DstExampleProcessor() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

void DstExampleProcessor::Init() {
    LOG << "DstExampleProcessor::Init" << LOG_END;
    // Open TFiles, set up TTree branches, etc
}

void DstExampleProcessor::Process(const JEvent& event) {
    LOG << "DstExampleProcessor::Process, Event #" << event.GetEventNumber() << LOG_END;

    /// Note that GetAllChildren won't trigger any new computations, it will only
    /// project down results which already exist in the JEvent. In order to obtain
    /// results from our DstExampleFactory, we need to trigger it explicitly using
    /// either JEvent::Get or JEvent::GetAll.

    event.Get<MyRenderableJObject>("from_factory");

    /// Now we can project our event down to a map of Renderables and a separate
    /// map of JObjects. Note we do this in parallel.
    auto renderable_map = event.GetAllChildren<Renderable>();
    auto jobject_map = event.GetAllChildren<JObject>();

    /// Senquentially, we iterate over all of our Renderables and JObjects and use
    /// whatever functionality each interface provides.

    std::lock_guard<std::mutex>lock(m_mutex);
    for (const auto& item : renderable_map) {
        // destructure
        std::string factory_name = item.first.first;
        std::string factory_tag = item.first.second;
        const std::vector<Renderable*>& renderables = item.second;

        LOG << "----------------------------------" << LOG_END;
        LOG << "Found factory producing Renderables: " << factory_name << ", " << factory_tag << LOG_END;
        for (auto renderable : renderables) {
            renderable->Render();
        }
    }

    for (const auto& item : jobject_map) {
        // destructure
        std::string factory_name = item.first.first;
        std::string factory_tag = item.first.second;
        const std::vector<JObject*>& jobjects = item.second;

        LOG << "----------------------------------" << LOG_END;
        LOG << "Found factory producing JObjects: " << factory_name << ", " << factory_tag << LOG_END;

        for (auto jobject : jobjects) {
            LOG << "Found JObject with classname " << jobject->className() << LOG_END;
        }
    }
    LOG << "----------------------------------" << LOG_END;
}

void DstExampleProcessor::Finish() {
    // Close any resources
    LOG << "DstExampleProcessor::Finish" << LOG_END;
}

