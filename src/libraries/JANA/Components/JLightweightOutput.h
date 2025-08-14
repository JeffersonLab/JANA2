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

} // jana::components

template <typename T> using Output = jana::components::Output<T>;
