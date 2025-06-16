#pragma once
#include <JANA/Components/JHasDatabundleOutputs.h>
#include <JANA/JEvent.h>

namespace jana::components {

template <typename T>
class Output : public JHasDatabundleOutputs::OutputBase {
    std::vector<T*> m_data;

public:
    Output(JHasDatabundleOutputs* owner, std::string default_tag_name="") {
        owner->RegisterOutput(this);
        this->collection_names.push_back(default_tag_name);
        this->type_name = JTypeInfo::demangle<T>();
    }

    std::vector<T*>& operator()() { return m_data; }

protected:
    void PutCollections(const JEvent& event) override {
        event.Insert(m_data, this->collection_names[0]);
    }
    void Reset() override { }
};

} // jana::components
