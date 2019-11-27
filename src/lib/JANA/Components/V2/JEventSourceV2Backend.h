//
// Created by Nathan Brei on 2019-11-24.
//

#ifndef JANA2_JEVENTSOURCEBACKENDV2_H
#define JANA2_JEVENTSOURCEBACKENDV2_H

#include <JANA/Components/JEventSourceBackend.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEvent.h>

namespace jana {
namespace v2 {

class JEventSourceV2Backend : public jana::components::JEventSourceBackend {
private:
    jana::v2::JEventSource* m_frontend;

public:
    JEventSourceV2Backend(jana::v2::JEventSource* frontend) : m_frontend(frontend){}

    JFactoryGenerator* m_factory_generator = nullptr;

    void open() override {
        m_frontend->Open();
    }

    Result next(JEvent &event) override {
        m_frontend->GetEvent(event.shared_from_this());
        return Result::FailureFinished;
    }
};


} // namespace v2
} // namespace jana


#endif //JANA2_JEVENTSOURCEBACKENDV2_H

