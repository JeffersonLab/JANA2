// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JService.h>
#include <JANA/JException.h>


void JService::DoInit(JServiceLocator* sl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (this->m_status != Status::Uninitialized) return; 
    try {
        for (auto* parameter : m_parameters) {
            parameter->Configure(*(m_app->GetJParameterManager()), m_prefix);
        }
        for (auto* service : m_services) {
            service->Init(GetApplication());  // TODO: This badly needs to be renamed. Maybe Fetch?
        }
        this->acquire_services(sl);
        this->Init();
        this->m_status = Status::Initialized;
    }
    catch (JException& ex) {
        ex.plugin_name = m_plugin_name;
        ex.component_name = m_type_name;
        throw ex;
    }
    catch (std::exception& e){
        auto ex = JException("Exception in JService::DoInit(): %s", e.what());
        ex.nested_exception = std::current_exception();
        ex.plugin_name = m_plugin_name;
        ex.component_name = m_type_name;
        throw ex;
    }
    catch (...) {
        auto ex = JException("Unknown exception in JService::DoInit()");
        ex.nested_exception = std::current_exception();
        ex.plugin_name = m_plugin_name;
        ex.component_name = m_type_name;
        throw ex;
    }
}
