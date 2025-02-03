
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/JApplication.h>
#include <JANA/Components/JComponentFwd.h>
#include <JANA/Utils/JTypeInfo.h>

namespace jana::components {

template <typename T>
class JComponent::ParameterRef : public JComponent::ParameterBase {

    T* m_data;

public:
    ParameterRef(JComponent* owner, std::string name, T& slot, std::string description="", bool is_shared=false) {
        owner->RegisterParameter(this);
        this->m_name = name;
        this->m_description = description;
        this->m_is_shared = is_shared;
        m_data = &slot;
    }

    void SetRef(T* slot) { m_data = slot; }

    const T& operator()() { return *m_data; }
    const T& operator*() { return *m_data; }

protected:

    void Init(JParameterManager& parman, const std::string& prefix) override {
        if (m_is_shared || prefix.empty()) {
            parman.SetDefaultParameter(this->m_name, *m_data, this->m_description);
        }
        else {
            parman.SetDefaultParameter(prefix + ":" + this->m_name, *m_data, this->m_description);
        }
    }
    void Wire(const std::map<std::string, std::string>& isolated, const std::map<std::string, std::string>& shared) override {
        if (m_is_shared) {
            auto it = shared.find(this->m_name);
            if (it != shared.end()) {
                const auto& value_str = it->second;
                JParameterManager::Parse(value_str, *m_data);
            }
        }
        else {
            auto it = isolated.find(this->m_name);
            if (it != isolated.end()) {
                const auto& value_str = it->second;
                JParameterManager::Parse(value_str, *m_data);
            }
        }
    }
};

template <typename T>
class JComponent::Parameter : public JComponent::ParameterBase {

    T m_data;

public:
    Parameter(JComponent* owner, std::string name, T default_value, std::string description="", bool is_shared=false) {
        owner->RegisterParameter(this);
        this->m_name = name;
        this->m_description = description;
        this->m_is_shared = is_shared;
        m_data = default_value;
    }

    const T& operator()() { return m_data; }
    const T& operator*() { return m_data; }

protected:

    void Init(JParameterManager& parman, const std::string& prefix) override {
        if (m_is_shared || prefix.empty()) {
            parman.SetDefaultParameter(this->m_name, m_data, this->m_description);
        }
        else {
            parman.SetDefaultParameter(prefix + ":" + this->m_name, m_data, this->m_description);
        }
    }
    void Wire(const std::map<std::string, std::string>& isolated, const std::map<std::string, std::string>& shared) override {
        if (m_is_shared) {
            auto it = shared.find(this->m_name);
            if (it != shared.end()) {
                const auto& value_str = it->second;
                JParameterManager::Parse(value_str, m_data);
            }
        }
        else {
            auto it = isolated.find(this->m_name);
            if (it != isolated.end()) {
                const auto& value_str = it->second;
                JParameterManager::Parse(value_str, m_data);
            }
        }
    }
};


template <typename ServiceT>
class JComponent::Service : public JComponent::ServiceBase {

    std::shared_ptr<ServiceT> m_data;

public:

    Service(JComponent* owner) {
        owner->RegisterService(this);
    }

    ServiceT& operator()() {
        if (m_data == nullptr) {
            throw JException("Attempted to access a Service which hasn't been attached to this Component yet!");
        }
        return *m_data;
    }

    ServiceT* operator->() {
        if (m_data == nullptr) {
            throw JException("Attempted to access a Service which hasn't been attached to this Component yet!");
        }
        return m_data.get();
    }

protected:

    void Fetch(JApplication* app) {
        m_data = app->GetService<ServiceT>();
    }
};


template <typename F> 
inline void JComponent::CallWithJExceptionWrapper(std::string func_name, F func) {
    try {
        func();
    }
    catch (JException& ex) {
        if (ex.function_name.empty()) ex.function_name = func_name;
        if (ex.type_name.empty()) ex.type_name = m_type_name;
        if (ex.instance_name.empty()) ex.instance_name = m_prefix;
        if (ex.plugin_name.empty()) ex.plugin_name = m_plugin_name;
        throw ex;
    }
    catch (std::exception& e) {
        auto ex = JException(e.what());
        ex.exception_type = JTypeInfo::demangle_current_exception_type();
        ex.nested_exception = std::current_exception();
        ex.function_name = func_name;
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    catch (...) {
        auto ex = JException("Unknown exception");
        ex.exception_type = JTypeInfo::demangle_current_exception_type();
        ex.nested_exception = std::current_exception();
        ex.function_name = func_name;
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
}


} // namespace jana::components


