
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JApplicationFwd.h>

#include <JANA/Services/JServiceLocator.h>
#include <JANA/Services/JParameterManager.h>


/// A convenience method which delegates to JParameterManager
template<typename T>
T JApplication::GetParameterValue(std::string name) {
    return m_params->GetParameterValue<T>(name);
}

/// A convenience method which delegates to JParameterManager
template<typename T>
JParameter* JApplication::SetParameterValue(std::string name, T val) {
    if (m_initialized) {
        throw JException("SetParameterValue() must be called before Initialize(), as otherwise the parameter value won't be used!");
    }
    return m_params->SetParameter(name, val);
}

template <typename T>
JParameter* JApplication::SetDefaultParameter(std::string name, T& val, std::string description) {
    return m_params->SetDefaultParameter(name.c_str(), val, description);
}

template <typename T>
T JApplication::RegisterParameter(std::string name, const T default_val, std::string description) {
    return m_params->RegisterParameter(name.c_str(), default_val, description);
}

template <typename T>
JParameter* JApplication::GetParameter(std::string name, T& result) {
    return m_params->GetParameter(name, result);
}

/// A convenience method which delegates to JServiceLocator
template <typename T>
std::shared_ptr<T> JApplication::GetService() {
    if (!m_services_available) {
        LOG_WARN(m_logger) << "GetService() called before Initialize(): Any parameter values set after this point won't be used!" << LOG_END;
        // Eventually, GetService() could trigger Initialize() just like Run() does. 
        // In order to make this happen, JTopologyBuilder needs modification.
        // The blockers are SubeventExample, TopologyTests, SubeventTests
        //throw JException("Application needs initialization before services become available");
    }
    return m_service_locator->get<T>();
}

/// A convenience method which delegates to JServiceLocator
template <typename T>
void JApplication::ProvideService(std::shared_ptr<T> service) {
    if (m_initialized) {
        throw JException("Services need to be provided before JApplication::Initialize(), or inside InitPlugin()");
    }
    service->SetApplication(this);
    m_service_locator->provide(service);
}



