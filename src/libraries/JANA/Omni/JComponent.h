
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/JApplication.h>
#include <JANA/Omni/JComponentFwd.h>

namespace jana {
namespace omni {

template <typename T>
class JComponent::ParameterRef : public JComponent::ParameterBase {

    T* m_data;

public:
    ParameterRef(JComponent* owner, std::string name, T& slot, std::string description="") {
        owner->RegisterParameter(this);
        this->m_name = name;
        this->m_description = description;
        m_data = &slot;
    }

    const T& operator()() { return *m_data; }

protected:

    void Configure(JParameterManager& parman, const std::string& prefix) override {
        parman.SetDefaultParameter(prefix + ":" + this->m_name, *m_data, this->m_description);
    }
    void Configure(std::map<std::string, std::string> fields) override {
        auto it = fields.find(this->m_name);
        if (it != fields.end()) {
            const auto& value_str = it->second;
            JParameterManager::Parse(value_str, *m_data);
        }
    }
};

template <typename T>
class JComponent::Parameter : public JComponent::ParameterBase {

    T m_data;

public:
    Parameter(JComponent* owner, std::string name, T default_value, std::string description) {
        owner->RegisterParameter(this);
        this->m_name = name;
        this->m_description = description;
        m_data = default_value;
    }

    const T& operator()() { return m_data; }

protected:

    void Configure(JParameterManager& parman, const std::string& prefix) override {
        parman.SetDefaultParameter(prefix + ":" + this->m_name, m_data, this->m_description);
    }
    void Configure(std::map<std::string, std::string> fields) override {
        auto it = fields.find(this->m_name);
        if (it != fields.end()) {
            const auto& value_str = it->second;
            JParameterManager::Parse(value_str, m_data);
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
        return *m_data;
    }

protected:

    void Init(JApplication* app) {
        m_data = app->GetService<ServiceT>();
    }
};



} // namespace omni
} // namespace jana


