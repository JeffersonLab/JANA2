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
        try {
            std::call_once(m_init_flag, &JEventSource::Open, m_frontend);
            m_status = Status::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->GetName();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSource::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->GetName();
            throw ex;
        }
    }

    Result next(JEvent &event) override {

        try {
            switch (m_status) {
                case Status::Unopened: open(); // Fall-through to Opened afterwards
                case Status::Opened:   m_frontend->GetEvent(event.shared_from_this());
                                       ++m_event_count;
                                       return Result::Success;
                case Status::Finished: return Result::FailureFinished;
            }
        }
        catch (JEventSource::RETURN_STATUS rs) {
            switch(rs) {
                case JEventSource::RETURN_STATUS::kNO_MORE_EVENTS :
                    m_status = Status::Finished;
                    return Result::FailureFinished;

                case JEventSource::RETURN_STATUS::kSUCCESS:
                    return Result::Success;

                case JEventSource::RETURN_STATUS::kERROR:
                    throw JException("Unknown error in JEventSource!");

                default:
                    return Result::FailureTryAgain;
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->GetTypeName();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSource::GetEvent()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->GetTypeName();
            throw ex;
        }
    }
};


} // namespace v2
} // namespace jana


#endif //JANA2_JEVENTSOURCEBACKENDV2_H

