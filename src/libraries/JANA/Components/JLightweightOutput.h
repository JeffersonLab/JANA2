#pragma once
#include "JANA/Components/JDatabundle.h"
#include "JANA/JFactorySet.h"
#include <JANA/Components/JHasOutputs.h>
#include <JANA/Components/JLightweightDatabundle.h>
#include <JANA/Utils/JTypeInfo.h>
#include <typeindex>

namespace jana::components {

template <typename T>
class Output : public JHasOutputs::OutputBase {
    std::vector<T*> m_transient_data;
    std::vector<T*>* m_external_data = nullptr; // This is a hack for JFactoryT
    JLightweightDatabundleT<T>* m_databundle; // Just so that we have a typed reference

public:
    Output(JHasOutputs* owner, std::string short_name="") {
        owner->RegisterOutput(this);
        m_databundle = new JLightweightDatabundleT<T>();
        m_databundle->SetTypeName(JTypeInfo::demangle<T>());
        m_databundle->SetTypeIndex(std::type_index(typeid(T)));
        m_databundle->SetShortName(short_name);
        SetDatabundle(m_databundle);
        // Factory will be set by JFactorySet, not here
    }

    Output(JHasOutputs* owner, std::vector<T*>* external_data, std::string short_name="") {
        owner->RegisterOutput(this);
        m_databundle = new JLightweightDatabundleT<T>(external_data);
        m_databundle->SetTypeName(JTypeInfo::demangle<T>());
        m_databundle->SetTypeIndex(std::type_index(typeid(T)));
        m_databundle->SetShortName(short_name);
        m_external_data = external_data;
        SetDatabundle(m_databundle);
        // Factory will be set by JFactorySet, not here
    }

    void SetNotOwnerFlag(bool not_owner) {
        m_databundle->SetNotOwnerFlag(not_owner);
    }

    std::vector<T*>& operator()() { return (m_external_data == nullptr) ? m_transient_data : *m_external_data; }

    JLightweightDatabundleT<T>& GetDatabundle() { return *m_databundle; }

    void LagrangianStore(JFactorySet&, JDatabundle::Status status) override {
        if (m_external_data == nullptr) {
            m_databundle->GetData() = std::move(m_transient_data);
        }
        m_databundle->SetStatus(status);
    }

    void EulerianStore(JFactorySet& facset) override {
        JLightweightDatabundleT<T>* typed_bundle = nullptr;
        auto bundle = facset.GetDatabundle(m_databundle->GetTypeIndex(), m_databundle->GetUniqueName());

        if (bundle == nullptr) {
            // No databundle present. In this case we simply use m_podio_databundle as a template
            typed_bundle = new JLightweightDatabundleT<T>(*m_databundle);
            facset.Add(typed_bundle);
        }
        else {
            typed_bundle = dynamic_cast<JLightweightDatabundleT<T>*>(bundle);
            if (typed_bundle == nullptr) {
                // Wrong databundle present
                throw JException("Databundle with unique_name '%s' is not a JLightweightDatabundleT<%s>", m_databundle->GetUniqueName().c_str(), m_databundle->GetTypeName().c_str());
            }
        }
        if (m_external_data == nullptr) {
            typed_bundle->GetData() = std::move(m_transient_data);
        }
        else {
            typed_bundle->GetData() = std::move(*m_external_data);
        }
        typed_bundle->SetStatus(JDatabundle::Status::Inserted);
    }
};


template <typename T>
class VariadicOutput : public JHasOutputs::VariadicOutputBase {
    std::vector<std::vector<T*>> m_transient_datas;
    bool m_not_owner_flag = false;

public:
    VariadicOutput(JHasOutputs* owner, std::string short_name="") {
        owner->RegisterOutput(this);
        auto* databundle = new JLightweightDatabundleT<T>();
        databundle->SetTypeName(JTypeInfo::demangle<T>());
        databundle->SetTypeIndex(std::type_index(typeid(T)));
        databundle->SetShortName(short_name);
        GetDatabundles().push_back(databundle);
        // Factory will be set by JFactorySet, not here
    }

    void SetNotOwnerFlag(bool not_owner) {
        m_not_owner_flag = not_owner;
        // We store this so that we can reapply it in case the user
        // calls SetShortNames() afterwards
        for (auto* db : GetDatabundles()) {
            auto typed_db = static_cast<JLightweightDatabundleT<T>*>(db);
            typed_db->SetNotOwnerFlag(not_owner);
        }
    }

    std::vector<std::vector<T*>>& operator()() { return m_transient_datas; }
    std::vector<std::vector<T*>>* operator->() { return &m_transient_datas; }

    void SetShortNames(std::vector<std::string> short_names) override {
        for (auto& db : GetDatabundles()) {
            delete db;
        }
        GetDatabundles().clear();
        m_transient_datas.clear();
        for (const std::string& name : short_names) {
            auto databundle = new JLightweightDatabundleT<T>;
            databundle->SetShortName(name);
            databundle->SetTypeName(JTypeInfo::demangle<T>());
            databundle->SetTypeIndex(std::type_index(typeid(T)));
            databundle->SetNotOwnerFlag(m_not_owner_flag);
            GetDatabundles().push_back(databundle);
            m_transient_datas.push_back({});
        }
    }
    void SetUniqueNames(std::vector<std::string> unique_names) override {
        for (auto& db : GetDatabundles()) {
            delete db;
        }
        GetDatabundles().clear();
        m_transient_datas.clear();
        for (const std::string& name : unique_names) {
            auto databundle = new JLightweightDatabundleT<T>;
            databundle->SetUniqueName(name);
            databundle->SetTypeName(JTypeInfo::demangle<T>());
            databundle->SetTypeIndex(std::type_index(typeid(T)));
            databundle->SetNotOwnerFlag(m_not_owner_flag);
            GetDatabundles().push_back(databundle);
            m_transient_datas.push_back({});
        }

    }

    void LagrangianStore(JFactorySet&, JDatabundle::Status status) override {
        if (m_transient_datas.size() != GetDatabundles().size()) {
            throw JException("Wrong number of output vectors in variadic output: Expected %d, found %d", GetDatabundles().size(), m_transient_datas.size());
        }
        size_t i=0;
        for (auto* databundle : GetDatabundles()) {
            JLightweightDatabundleT<T>* typed_bundle = dynamic_cast<JLightweightDatabundleT<T>*>(databundle);
            if (typed_bundle == nullptr) {
                throw JException("Databundle is not a JLightweightDatabundleT (Hint: This is an internal error)");
            }
            typed_bundle->GetData() = std::move(m_transient_datas.at(i));
            typed_bundle->SetStatus(status);
            i += 1;
        }
    }

    void EulerianStore(JFactorySet& facset) override {

        if (m_transient_datas.size() != GetDatabundles().size()) {
            throw JException("Wrong number of output vectors in variadic output: Expected %d, found %d", GetDatabundles().size(), m_transient_datas.size());
        }
        size_t i=0;
        for (auto* databundle_prototype : GetDatabundles()) {
            JLightweightDatabundleT<T>* typed_databundle = nullptr;
            auto* databundle = facset.GetDatabundle(databundle_prototype->GetUniqueName());
            if (databundle == nullptr) {
                auto* typed_databundle_prototype = dynamic_cast<JLightweightDatabundleT<T>*>(databundle_prototype);
                typed_databundle = new JLightweightDatabundleT<T>(*typed_databundle_prototype);
                facset.Add(typed_databundle);
            }
            else {
                typed_databundle = dynamic_cast<JLightweightDatabundleT<T>*>(databundle);
                if (typed_databundle == nullptr) {
                    throw JException("Databundle with unique_name '%s' is not a JLightweightDatabundle<%s>", databundle_prototype->GetUniqueName().c_str(), JTypeInfo::demangle<T>().c_str());
                }
            }
            typed_databundle->GetData() = std::move(m_transient_datas.at(i));
            typed_databundle->SetStatus(JDatabundle::Status::Inserted);
            i += 1;
        }
    }
};

} // jana::components

template <typename T> using Output = jana::components::Output<T>;
template <typename T> using VariadicOutput = jana::components::VariadicOutput<T>;
