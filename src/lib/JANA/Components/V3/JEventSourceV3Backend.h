//
// Created by Nathan Brei on 2019-11-24.
//

#ifndef JANA2_JEVENTSOURCEV3BACKEND_H
#define JANA2_JEVENTSOURCEV3BACKEND_H

#include <JANA/Components/JEventSourceBackend.h>
#include <JANA/Components/JEventSourceFrontend.h>
#include <JANA/JEvent.h>

namespace jana {
namespace v3 {

class JEventSourceV3Backend : public JEventSourceBackend {
private:
    jana::v3::JEventSource* m_frontend;
    std::once_flag m_closed_flag;

public:
    JEventSourceV3Backend(JEventSource* frontend) : m_frontend(frontend) {};
    void open() override {
        try {
            std::call_once(m_init_flag, &JEventSource::open, m_frontend, m_resource_name);
            m_status = Status::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->get_type_name();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSource::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->get_type_name();
            throw ex;
        }

    }


    Result next(JEvent &event) override {

        try {
            switch (m_status) {
                case Status::Unopened:
                    open(); // Fall-through to Opened afterwards

                case Status::Opened: {
                    auto result = m_frontend->next_event(event);
                    switch (result) {
                        case jana::v3::JEventSource::Result::SUCCESS:
                            ++m_event_count;
                            return Result::Success;

                        case jana::v3::JEventSource::Result::FAILURE_BUSY:
                            return Result::FailureTryAgain;

                        case jana::v3::JEventSource::Result::FAILURE_FINISHED:
                            // This is the first time the source has encountered FINISHED
                            std::call_once(m_closed_flag, &JEventSource::close, m_frontend);
                            m_status = Status::Finished;
                            return Result::FailureFinished;
                    }
                }
                case Status::Finished:
                    return Result::FailureFinished;
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->get_type_name();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSource::GetEvent()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = m_frontend->get_type_name();
            throw ex;
        }
    }

};

}
}
#endif //JANA2_JEVENTSOURCEV3BACKEND_H
