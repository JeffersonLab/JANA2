
#include "DstExampleProcessor.h"
#include "DataObjects.h"
#include <JANA/JLogger.h>

DstExampleProcessor::DstExampleProcessor() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}

void DstExampleProcessor::Init() {
    LOG << "DstExampleProcessor::Init" << LOG_END;
    // Open TFiles, set up TTree branches, etc
}

void DstExampleProcessor::Process(const std::shared_ptr<const JEvent> &event) {
    LOG << "DstExampleProcessor::Process, Event #" << event->GetEventNumber() << LOG_END;

    /// Obtain all Renderables and all JObjects, in parallel
    auto renderable_map = event->GetAllChildren<Renderable>();
    auto jobject_map = event->GetAllChildren<JObject>();

    /// Senquentially, iterate over everything
    std::lock_guard<std::mutex>lock(m_mutex);
    for (const auto& item : renderable_map) {
        // destructure
        std::string factory_name = item.first.first;
        std::string factory_tag = item.first.second;
        const std::vector<Renderable*>& renderables = item.second;

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

        LOG << "Found factory producing JObjects: " << factory_name << ", " << factory_tag << LOG_END;

        for (auto jobject : jobjects) {
            LOG << "Found JObject with classname " << jobject->className() << LOG_END;
        }
    }
}

void DstExampleProcessor::Finish() {
    // Close any resources
    LOG << "DstExampleProcessor::Finish" << LOG_END;
}

