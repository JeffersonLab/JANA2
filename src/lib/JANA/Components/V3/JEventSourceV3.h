//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_JEVENTSOURCEV3_H
#define JANA2_JEVENTSOURCEV3_H

#include <JANA/Components/JAbstractEventSource.h>

class JEventSourceV3 : public JAbstractEventSource {

public:

    // ----------------------------------
    // User is supposed to override these
    // ----------------------------------

    enum class Result { Success, FailureTryAgain, FailureFinished };

    virtual void Open(const std::string& resource_name, JApplication* app) {}

    virtual Result Next(JEvent& event) = 0;

    virtual void Close() {}



    // ------------------------
    // Used internally by JANA
    // ------------------------
    // Everything below here should be in a JEventSourceBackend instead


    void DoInitialize() final {
        try {
            std::call_once(m_init_flag, &JEventSourceV3::Open, this, this->m_resource_name, this->m_application);
            m_status = SourceStatus::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSourceV3::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    // TODO: Right now, nobody calls DoClose(). We need DoClose in the case where we are consuming a stream
    //       from a network socket and somebody calls Stop(). We want the socket to be released cleanly.

    void DoClose() {
        try {
            std::call_once(m_finish_flag, &JEventSourceV3::Close, this);
            m_status = SourceStatus::Finished;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSourceV3::Close()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    ReturnStatus DoNext(std::shared_ptr<JEvent> event) final {
        Result result;
        try {
            switch (m_status) {

                case SourceStatus::Unopened:
                    DoInitialize(); // Fall-through to Opened afterwards

                case SourceStatus::Opened:

                    switch (Next(*event)) {
                        case Result::Success:
                            return ReturnStatus::Success;

                        case Result::FailureTryAgain:
                            return ReturnStatus::TryAgain;

                        case Result::FailureFinished:
                            m_status = SourceStatus::Finished;
                            std::call_once(m_finish_flag, &JEventSourceV3::Close, this);
                            return ReturnStatus::Finished;

                        default:
                            throw JException("Invalid ReturnStatus");
                    }

                case SourceStatus::Finished:
                    return ReturnStatus::Finished;
            }
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventSourceV2::GetEvent()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

private:
    std::once_flag m_init_flag;
    std::once_flag m_finish_flag;
};



#endif //JANA2_JEVENTSOURCEV3_H
