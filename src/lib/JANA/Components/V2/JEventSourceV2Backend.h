//
// Created by Nathan Brei on 2019-11-24.
//

#ifndef JANA2_JEVENTSOURCEBACKENDV2_H
#define JANA2_JEVENTSOURCEBACKENDV2_H

#include <JANA/Components/JEventSourceBackend.h>
#include <JANA/JFactoryGenerator.h>

namespace jana {
namespace v2 {

struct JEventSourceV2Backend : public jana::components::JEventSourceBackend {

public:

    JEventSourceV2Backend(std::string resource_name) {
        m_resource_name = resource_name;
    }

    JFactoryGenerator* m_factory_generator = nullptr;

    void open() override {

    }

    Result next(JEvent &) override {

    }
};

} // namespace v2
} // namespace jana


#endif //JANA2_JEVENTSOURCEBACKENDV2_H

