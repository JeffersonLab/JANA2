//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_JEVENTPROCESSORV3_H
#define JANA2_JEVENTPROCESSORV3_H

#include <JANA/Components/JAbstractEventProcessor.h>
#include <JANA/JException.h>
#include <JANA/JEvent.h>

class JEventProcessorV3 : public JAbstractEventProcessor {
public:

    // ----------------------------------
    // User is supposed to override these
    // ----------------------------------

    virtual void Init(JApplication* app) {}

    virtual void ProcessParallel(const JEvent&) {}

    virtual void ProcessSequential(const JEvent&) {}

    virtual void Finish() {}

    // TODO: Improve me!
    // Tell the user to call SetType(), SetGlobalLock()
    virtual std::string GetType() const { return m_type_name; }


    // ------------------------
    // Used internally by JANA
    // ------------------------

    void DoInitialize() final {
        try {
            std::call_once(m_init_flag, &JEventProcessorV3::Init, this, this->mApplication);
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
            ProcessParallel(*e);
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessorV3::DoMap()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    void DoReduce(const std::shared_ptr<const JEvent>& e) final {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);
            ProcessSequential(*e);
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessorV3::DoReduce()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

    void DoFinalize() final {
        try {
            std::call_once(m_finish_flag, &JEventProcessorV3::Finish, this);
            m_status = Status::Finished;
        }
        catch (JException& ex) {
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JEventProcessorV3::Finish()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = m_plugin_name;
            ex.component_name = GetType();
            throw ex;
        }
    }

private:
    std::once_flag m_init_flag;
    std::once_flag m_finish_flag;
    std::mutex m_mutex;
};



#endif //JANA2_JEVENTPROCESSORV3_H
