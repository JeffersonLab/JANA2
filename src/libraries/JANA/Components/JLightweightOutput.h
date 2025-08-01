#pragma once
#include "JANA/Components/JDatabundle.h"
#include <JANA/Components/JHasDatabundleOutputs.h>
#include <JANA/Components/JLightweightDatabundle.h>
#include <JANA/Utils/JTypeInfo.h>
#include <typeindex>

namespace jana::components {

template <typename T>
class Output : public JHasDatabundleOutputs::OutputBase {
    std::vector<T*> m_transient_data;
    std::vector<T*>* m_external_data = nullptr; // This is a hack for JFactoryT
    JLightweightDatabundleT<T>* m_databundle; // Just so that we have a typed reference

public:
    Output(JHasDatabundleOutputs* owner, std::string short_name="") {
        owner->RegisterOutput(this);
        m_databundle = new JLightweightDatabundleT<T>();
        m_databundle->SetTypeName(JTypeInfo::demangle<T>());
        m_databundle->SetTypeIndex(std::type_index(typeid(T)));
        m_databundle->SetShortName(short_name);
        SetDatabundle(m_databundle);
        // Factory will be set by JFactorySet, not here
    }

    Output(JHasDatabundleOutputs* owner, std::vector<T*>* external_data, std::string short_name="") {
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
};

} // jana::components
