#pragma once
#include "JANA/Components/JDatabundle.h"
#include <JANA/Components/JHasDatabundleOutputs.h>
#include <JANA/Components/JLightweightDatabundle.h>
#include <JANA/Utils/JTypeInfo.h>
#include <typeindex>

namespace jana::components {

template <typename T>
class Output : public JHasDatabundleOutputs::OutputBase {
    JLightweightDatabundleT<T>* m_databundle; // Just so that we have a typed reference

public:
    Output(JHasDatabundleOutputs* owner, std::string short_name="") {
        owner->RegisterOutput(this);
        m_databundle = new JLightweightDatabundleT<T>();
        m_databundle->SetTypeName(JTypeInfo::demangle<T>());
        m_databundle->SetTypeIndex(std::type_index(typeid(T)));
        m_databundle->SetShortName(short_name);
        m_databundle->UseSelfContainedData();
        GetDatabundles().push_back(m_databundle);
        // Factory will be set by JFactorySet, not here
    }

    void SetShortName(std::string short_name) {
        m_databundle->SetShortName(short_name);
    }

    void SetUniqueName(std::string unique_name) {
        m_databundle->SetUniqueName(unique_name);
    }

    void SetNotOwnerFlag(bool not_owner) {
        m_databundle->SetNotOwnerFlag(not_owner);
    }

    std::vector<T*>& operator()() { return m_databundle->GetData(); }

    JLightweightDatabundleT<T>& GetDatabundle() { return *m_databundle; }

    void StoreData(JFactorySet&) override {
        m_databundle->SetStatus(JDatabundle::Status::Created);
    }

    void Reset() override { }
};

} // jana::components
