#pragma once
#include "JANA/Components/JDatabundle.h"
#include <JANA/Components/JHasDatabundleOutputs.h>
#include <JANA/Components/JLightweightDatabundle.h>
#include <JANA/Utils/JTypeInfo.h>

namespace jana::components {

template <typename T>
class Output : public JHasDatabundleOutputs::OutputBase {
    std::vector<T*> m_data;
    JLightweightDatabundleT<T>* m_databundle; // Just so that we have a typed reference

public:
    Output(JHasDatabundleOutputs* owner, std::string short_name="") {
        owner->RegisterOutput(this);
        this->databundle_names.push_back(short_name);
        this->type_name = JTypeInfo::demangle<T>();
        m_databundle = new JLightweightDatabundleT<T>();
        m_databundle->SetTypeName(this->type_name);
        std::string unique_name = short_name.empty() ? this->type_name : this->type_name + ":" + short_name;
        m_databundle->SetUniqueName(unique_name);
        this->databundles.push_back(m_databundle);
        // Factory will be set by JFactorySet, not here
    }

    std::vector<T*>& operator()() { return m_data; }

    JLightweightDatabundleT<T>& GetDatabundle() { return *m_databundle; }

    void StoreData(const JFactorySet&) override {
    //    event.Insert(m_data, this->collection_names[0]);
    }

    void Reset() override { }
};

} // jana::components
