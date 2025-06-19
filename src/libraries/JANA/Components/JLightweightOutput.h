#pragma once
#include <JANA/Components/JHasDatabundleOutputs.h>
#include <JANA/Utils/JTypeInfo.h>

namespace jana::components {

template <typename T>
class Output : public JHasDatabundleOutputs::OutputBase {
    std::vector<T*> m_data;

public:
    Output(JHasDatabundleOutputs* owner, std::string default_tag_name="") {
        owner->RegisterOutput(this);
        this->databundle_names.push_back(default_tag_name);
        this->type_name = JTypeInfo::demangle<T>();
    }

    std::vector<T*>& operator()() { return m_data; }

    void StoreData(const JFactorySet&) override {
    //    event.Insert(m_data, this->collection_names[0]);
    }

    void Reset() override { }
};

} // jana::components
