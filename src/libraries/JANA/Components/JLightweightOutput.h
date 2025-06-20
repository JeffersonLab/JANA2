#pragma once
#include <JANA/Components/JHasDatabundleOutputs.h>
#include <JANA/Components/JLightweightDatabundle.h>
#include <JANA/Utils/JTypeInfo.h>

namespace jana::components {

template <typename T>
class Output : public JHasDatabundleOutputs::OutputBase {
    std::vector<T*> m_data;

public:
    Output(JHasDatabundleOutputs* owner, std::string short_name="") {
        owner->RegisterOutput(this);
        this->databundle_names.push_back(short_name);
        this->type_name = JTypeInfo::demangle<T>();
        auto databundle = new JLightweightDatabundleT<T>();
        databundle->SetTypeName(this->type_name);
        std::string unique_name = short_name.empty() ? this->type_name : this->type_name + ":" + short_name;
        databundle->SetUniqueName(unique_name);
        this->databundles.push_back(databundle);
        // Factory will be set by JFactorySet, not here

        // TODO: Factory flags need to propagate from Output to JLightweightDatabundle
    }

    std::vector<T*>& operator()() { return m_data; }

    void StoreData(const JFactorySet&) override {
    //    event.Insert(m_data, this->collection_names[0]);
    }

    void Reset() override { }
};

} // jana::components
