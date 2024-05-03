// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JService.h>
#include <JANA/JException.h>


void JService::DoInit(JServiceLocator* sl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (this->m_status != Status::Uninitialized) return; 

    for (auto* parameter : m_parameters) {
        parameter->Configure(*(m_app->GetJParameterManager()), m_prefix);
    }

    for (auto* service : m_services) {
        service->Init(GetApplication());  // TODO: This badly needs to be renamed. Maybe Fetch?
    }

    CallWithJExceptionWrapper("JService::acquire_services", [&](){ this->acquire_services(sl); });
    CallWithJExceptionWrapper("JService::Init", [&](){ this->Init(); });

    this->m_status = Status::Initialized;
}
