//
// Created by Nathan Brei on 2019-11-24.
//

#ifndef JANA2_JEVENTSOURCEBACKENDV2_H
#define JANA2_JEVENTSOURCEBACKENDV2_H

namespace jana {
namespace v2 {

struct JEventSourceV2Backend : public JEventSourceBackend {

public JEventSourceV2Backend(std::string resource_name) : m_resource_name(resource_name) {}

    JFactoryGenerator* m_factory_generator = nullptr;

    virtual void open() {

    }

    virtual Result next(JEvent &) {

    }
};

} // namespace v2
} // namespace jana


#endif //JANA2_JEVENTSOURCEBACKENDV2_H

