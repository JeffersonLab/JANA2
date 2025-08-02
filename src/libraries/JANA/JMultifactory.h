
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JFactory.h>
#include <JANA/Components/JHasRunCallbacks.h>
#include <JANA/Components/JLightweightOutput.h>
#include <JANA/JVersion.h>
#include <typeindex>

#if JANA2_HAVE_PODIO
#include "JANA/Components/JPodioOutput.h"
#endif


class JMultifactory : public JFactory {

private:
    std::vector<OutputBase*> m_owned_outputs;
    std::map<std::pair<std::type_index, std::string>, OutputBase*> m_output_index;

public:
    JMultifactory() = default;
    virtual ~JMultifactory() {
        for (auto* output : m_owned_outputs) {
            delete output;
        }
    }

    // IMPLEMENTED BY USERS

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Finish() override {}


    // CALLED BY USERS

    template <typename T>
    void DeclareOutput(std::string short_name, bool owns_data=true);

    template <typename T>
    void SetData(std::string short_name, std::vector<T*> data);

#if JANA2_HAVE_PODIO

    template <typename T>
    void DeclarePodioOutput(std::string unique_name, bool owns_data=true);

    template <typename T>
    void SetCollection(std::string unique_name, typename T::collection_type&& collection);

    template <typename T>
    void SetCollection(std::string unique_name, std::unique_ptr<typename T::collection_type> collection);

#endif
};



template <typename T>
void JMultifactory::DeclareOutput(std::string short_name, bool owns_data) {

    auto* output = new jana::components::Output<T>(this);
    output->SetShortName(short_name);
    output->SetNotOwnerFlag(!owns_data);
    m_owned_outputs.push_back(output);
    m_output_index[{std::type_index(typeid(T)), short_name}] = output;
}

template <typename T>
void JMultifactory::SetData(std::string short_name, std::vector<T*> data) {
    auto it = m_output_index.find({std::type_index(typeid(T)), short_name});
    if (it == m_output_index.end()) {
        auto ex = JException("Couldn't find output with short_name '%s'. Hint: Did you call DeclareOutput() in the constructor?", short_name.c_str());
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    auto* typed_output = dynamic_cast<jana::components::Output<T>*>(it->second);
    if (typed_output == nullptr) {
        auto ex = JException("OutputBase not castable to Output<T>. Hint: Did you mean to use SetCollection?");
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    (*typed_output)() = std::move(data);
}


#if JANA2_HAVE_PODIO

template <typename T>
void JMultifactory::DeclarePodioOutput(std::string unique_name, bool owns_data) {
    auto* output = new jana::components::PodioOutput<T>(this);
    output->SetUniqueName(unique_name);
    output->SetSubsetCollection(!owns_data);
    m_owned_outputs.push_back(output);
    m_output_index[{std::type_index(typeid(T)), unique_name}] = output;
}

template <typename T>
void JMultifactory::SetCollection(std::string unique_name, typename T::collection_type&& collection) {
    auto it = m_output_index.find({std::type_index(typeid(T)), unique_name});
    if (it == m_output_index.end()) {
        auto ex = JException("Couldn't find output with short_name '%s'. Hint: Did you call DeclareOutput() in the constructor?", unique_name.c_str());
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    auto* typed_output = dynamic_cast<jana::components::PodioOutput<T>*>(it->second);
    if (typed_output == nullptr) {
        auto ex = JException("Databundle not castable to JLightweightDatabundleT. Hint: Did you mean to use SetCollection?");
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    *((*typed_output)()) = std::move(collection);
}

template <typename T>
void JMultifactory::SetCollection(std::string unique_name, std::unique_ptr<typename T::collection_type> collection) {
    auto it = m_output_index.find({std::type_index(typeid(T)), unique_name});
    if (it == m_output_index.end()) {
        auto ex = JException("Couldn't find output with short_name '%s'. Hint: Did you call DeclareOutput() in the constructor?", unique_name.c_str());
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    auto* typed_output = dynamic_cast<jana::components::PodioOutput<T>*>(it->second);
    if (typed_output == nullptr) {
        auto ex = JException("Databundle not castable to JLightweightDatabundleT. Hint: Did you mean to use SetCollection?");
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
    *((*typed_output)()) = std::move(*collection);
}

#endif // JANA2_HAVE_PODIO




