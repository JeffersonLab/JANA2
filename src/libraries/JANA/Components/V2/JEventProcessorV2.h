//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_JEVENTPROCESSORV2_H
#define JANA2_JEVENTPROCESSORV2_H

#include <JANA/Components/JAbstractEventProcessor.h>
#include <JANA/JException.h>
#include <JANA/JEvent.h>

class JEventProcessorV2 : public JAbstractEventProcessor {
public:

    // ----------------------------------
    // User is supposed to override these
    // ----------------------------------

    JEventProcessorV2(JApplication* app=nullptr) {
        mApplication = app;
    }

    virtual void Init() {}

    virtual void Process(const std::shared_ptr<const JEvent>& aEvent) {
        throw JException("Not implemented yet!");
    }

    virtual void Finish() {}

    virtual std::string GetType() const { return m_type_name; }


    // ------------------------
    // Used internally by JANA
    // ------------------------

    void DoInitialize() final {
        try {
            std::call_once(m_init_flag, &JEventProcessorV2::Init, this);
            m_status = Status::Opened;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessorV2::Open()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    void DoMap(const std::shared_ptr<const JEvent>& e) final {
        try {
            std::call_once(m_init_flag, &JAbstractEventProcessor::DoInitialize, this);
            Process(e);
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessorV2::DoMap()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }


    void DoFinalize() final {
        try {
            std::call_once(m_finish_flag, &JEventProcessorV2::Finish, this);
            m_status = Status::Finished;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessorV2::Finish()");
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


#endif //JANA2_JEVENTPROCESSORV2_H
