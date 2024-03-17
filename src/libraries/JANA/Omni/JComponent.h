
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/JEvent.h>

namespace jana {
namespace omni {


struct JComponent {
protected:

    struct ParameterBase;
    struct ServiceBase;
    struct ResourceBase;

    std::vector<ParameterBase*> m_parameters;
    std::vector<ServiceBase*> m_services;
    std::vector<ResourceBase*> m_resources;
    
    std::string m_prefix;

public:
    void SetPrefix(std::string prefix) {
        m_prefix = prefix;
    }


protected:
    struct ParameterBase {
        std::string m_name;
        std::string m_description;
        virtual void Configure(JParameterManager& parman, const std::string& prefix) = 0;
        virtual void Configure(std::map<std::string, std::string> fields) = 0;
    };

    struct ServiceBase {
        virtual void Init(JApplication* app) = 0;
    };

    struct ResourceBase {
        virtual void ChangeRun(const JEvent& event) = 0;
    };


    void RegisterParameter(ParameterBase* parameter) {
        m_parameters.push_back(parameter);
    }

    void RegisterService(ServiceBase* service) {
        m_services.push_back(service);
    }

    void RegisterResource(ResourceBase* resource) {
        m_resources.push_back(resource);
    }

    void ConfigureAllParameters(std::map<std::string, std::string> fields) {
        for (auto* parameter : this->m_parameters) {
            parameter->Configure(fields);
        }
    }
    template <typename T>
    class ParameterRef : public ParameterBase {

        T* m_data;

    public:
        ParameterRef(JComponent* owner, std::string name, T& slot, std::string description="") {
            owner->RegisterParameter(this);
            this->m_name = name;
            this->m_description = description;
            m_data = &slot;
        }

        const T& operator()() { return *m_data; }

    private:
        friend class JOmniFactory;

        void Configure(JParameterManager& parman, const std::string& prefix) override {
            parman.SetDefaultParameter(prefix + ":" + this->m_name, *m_data, this->m_description);
        }
        void Configure(std::map<std::string, std::string> fields) override {
            auto it = fields.find(this->m_name);
            if (it != fields.end()) {
                const auto& value_str = it->second;
                if constexpr (10000 * JVersion::major
                                + 100 * JVersion::minor
                                + 1 * JVersion::patch < 20102) {
                    *m_data = JParameterManager::Parse<T>(value_str);
                } else {
                    JParameterManager::Parse(value_str, *m_data);
                }
            }
        }
    };

    template <typename T>
    class Parameter : public ParameterBase {

        T m_data;

    public:
        Parameter(JOmniFactory* owner, std::string name, T default_value, std::string description) {
            owner->RegisterParameter(this);
            this->m_name = name;
            this->m_description = description;
            m_data = default_value;
        }

        const T& operator()() { return m_data; }

    private:
        friend class JOmniFactory;

        void Configure(JParameterManager& parman, const std::string& prefix) override {
            parman.SetDefaultParameter(m_prefix + ":" + this->m_name, m_data, this->m_description);
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
    class Service : public ServiceBase {

        std::shared_ptr<ServiceT> m_data;

    public:

        Service(JOmniFactory* owner) {
            owner->RegisterService(this);
        }

        ServiceT& operator()() {
            return *m_data;
        }

    private:

        friend class JOmniFactory;

        void Init(JApplication* app) {
            m_data = app->GetService<ServiceT>();
        }

    };



    template <typename ServiceT, typename ResourceT, typename LambdaT>
    class Resource : public ResourceBase {
        ResourceT m_data;
        LambdaT m_lambda;

    public:

        Resource(JOmniFactory* owner, LambdaT lambda) : m_lambda(lambda) {
            owner->RegisterResource(this);
        };

        const ResourceT& operator()() { return m_data; }

    private:
        friend class JOmniFactory;

        void ChangeRun(const JEvent& event) {
            auto run_nr = event.GetRunNumber();
            std::shared_ptr<ServiceT> service = event.GetJApplication()->template GetService<ServiceT>();
            m_data = m_lambda(service, run_nr);
        }
    };

}



} // namespace omni
} // namespace jana


