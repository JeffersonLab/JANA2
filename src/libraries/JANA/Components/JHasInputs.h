// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include "JANA/Components/JComponentSummary.h"
#include <JANA/JVersion.h>
#if JANA2_HAVE_PODIO
#include "JANA/Components/JPodioDatabundle.h"
#endif
#include "JANA/Components/JLightweightDatabundle.h"
#include "JANA/Utils/JEventLevel.h"
#include "JANA/Utils/JTypeInfo.h"
#include "JANA/JFactorySet.h"
#include <typeindex>


class JEvent;
namespace jana::components {

// Free function in order to break circular dependence on JEvent
JFactorySet* GetFactorySetAtLevel(const JEvent& event, JEventLevel desired_level);
void FactoryCreate(const JEvent& event, JFactory* factory);

struct JHasInputs {
protected:

    class InputBase;
    class VariadicInputBase;

    std::vector<InputBase*> m_inputs;
    std::vector<VariadicInputBase*> m_variadic_inputs;
    std::vector<std::pair<InputBase*, VariadicInputBase*>> m_ordered_inputs;


    void RegisterInput(InputBase* input) {
        m_inputs.push_back(input);
        m_ordered_inputs.push_back({input, nullptr});
    }

    void RegisterInput(VariadicInputBase* input) {
        m_variadic_inputs.push_back(input);
        m_ordered_inputs.push_back({nullptr, input});
    }

    const std::vector<InputBase*>& GetInputs() { 
        return m_inputs; 
    }

    const std::vector<VariadicInputBase*>& GetVariadicInputs() { 
        return m_variadic_inputs; 
    }

    struct InputOptions {
        std::string name {""};
        JEventLevel level {JEventLevel::None};
        bool is_optional {false};
    };

    struct VariadicInputOptions {
        std::vector<std::string> names {""};
        JEventLevel level {JEventLevel::None};
        bool is_optional {false};
    };

    class InputBase {
    protected:
        std::type_index m_type_index = std::type_index(typeid(JDatabundle::NoTypeProvided));
        std::string m_type_name;
        std::string m_databundle_name;
        JEventLevel m_level = JEventLevel::None;
        bool m_is_optional = false;

    public:

        virtual ~InputBase();

        void SetOptional(bool isOptional) {
            m_is_optional = isOptional;
        }

        void SetLevel(JEventLevel level) {
            m_level = level;
        }

        void SetDatabundleName(std::string name) {
            m_databundle_name = name;
        }

        const std::string& GetTypeName() const {
            return m_type_name;
        }

        const std::string& GetDatabundleName() const {
            return m_databundle_name;
        }

        JEventLevel GetLevel() const {
            return m_level;
        }

        void Configure(const InputOptions& options) {
            m_databundle_name = options.name;
            m_level = options.level;
            m_is_optional = options.is_optional;
        }

        void TriggerFactoryCreate(const JEvent& event);
        virtual void Populate(const JEvent& event) = 0;
    };

    class VariadicInputBase {
    public:
        enum class EmptyInputPolicy { IncludeNothing, IncludeEverything };

    protected:
        std::type_index m_type_index = std::type_index(typeid(JDatabundle::NoTypeProvided));
        std::string m_type_name;
        std::vector<std::string> m_requested_databundle_names;
        std::vector<std::string> m_realized_databundle_names;
        JEventLevel m_level = JEventLevel::None;
        bool m_is_optional = false;
        EmptyInputPolicy m_empty_input_policy = EmptyInputPolicy::IncludeNothing;

    public:

        virtual ~VariadicInputBase();

        void SetOptional(bool isOptional) {
            m_is_optional = isOptional;
        }

        void SetLevel(JEventLevel level) {
            m_level = level;
        }

        void SetRequestedDatabundleNames(std::vector<std::string> names) {
            m_requested_databundle_names = names;
            m_realized_databundle_names = names;
        }

        void SetEmptyInputPolicy(EmptyInputPolicy policy) {
            m_empty_input_policy = policy;
        }

        const std::string& GetTypeName() const {
            return m_type_name;
        }

        const std::vector<std::string>& GetRequestedDatabundleNames() const {
            return m_requested_databundle_names;
        }

        const std::vector<std::string>& GetRealizedDatabundleNames() const {
            return m_realized_databundle_names;
        }

        JEventLevel GetLevel() const {
            return m_level;
        }

        void Configure(const VariadicInputOptions& options) {
            m_requested_databundle_names = options.names;
            m_level = options.level;
            m_is_optional = options.is_optional;
        }

        void TriggerFactoryCreate(const JEvent& event);
        virtual void Populate(const JEvent& event) = 0;
    };


    template <typename T>
    class Input : public InputBase {

        std::vector<const T*> m_data;

    public:

        Input(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(T));
            m_type_name = JTypeInfo::demangle<T>();
            m_level = JEventLevel::None;
        }

        Input(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(T));
            m_type_name = JTypeInfo::demangle<T>();
            Configure(options);
        }

        void SetTag(std::string tag) {
            m_databundle_name = tag;
        }

        const std::vector<const T*>& operator()() { return m_data; }
        const std::vector<const T*>& operator*() { return m_data; }
        const std::vector<const T*>* operator->() { return &m_data; }


    private:
        friend class JComponentT;

        void Populate(const JEvent& event) {

            // Eventually, we might try maintaining a permanent link to the databundle
            // instead of having to retrieve it every time. This will only work if we are both on a
            // JFactory in the same JFactorySet, though.

            auto facset = GetFactorySetAtLevel(event, m_level);
            if (facset == nullptr) {
                if (m_is_optional) {
                    return;
                }
                throw JException("Could not find parent at level=" + toString(m_level));
            }
            auto databundle = facset->GetDatabundle(std::type_index(typeid(T)), m_databundle_name);
            if (databundle == nullptr) {
                if (!m_is_optional) {
                    facset->Print();
                    throw JException("Could not find databundle with type_index=" + JTypeInfo::demangle<T>() + " and tag=" + m_databundle_name);
                }
                m_data.clear();
                return;
            };
            auto* typed_databundle = dynamic_cast<JLightweightDatabundleT<T>*>(databundle);
            if (typed_databundle == nullptr) {
                if (!m_is_optional) {
                    facset->Print();
                    throw JException("Databundle with shortname '%s' does not inherit from JLightweightDatabundleT<%s>", m_databundle_name.c_str(), JTypeInfo::demangle<T>().c_str());
                }
            }
            m_data.clear();
            m_data.insert(m_data.end(), typed_databundle->GetData().begin(), typed_databundle->GetData().end());
        }
    };

#if JANA2_HAVE_PODIO
    template <typename PodioT>
    class PodioInput : public InputBase {

        const typename PodioT::collection_type* m_data;

    public:

        PodioInput(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(PodioT));
            m_type_name = JTypeInfo::demangle<PodioT>();
            m_databundle_name = m_type_name;
            m_level = JEventLevel::None;
        }

        PodioInput(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(PodioT));
            m_type_name = JTypeInfo::demangle<PodioT>();
            m_databundle_name = m_type_name;
            Configure(options);
        }

        const typename PodioT::collection_type* operator()() {
            return m_data;
        }
        const typename PodioT::collection_type& operator*() {
            return *m_data;
        }
        const typename PodioT::collection_type* operator->() {
            return m_data;
        }

        void SetCollectionName(std::string name) {
            m_databundle_name = name;
        }

        void SetTag(std::string tag) {
            m_databundle_name = m_type_name + ":" + tag;
        }

        void Populate(const JEvent& event) {
            auto facset = GetFactorySetAtLevel(event, m_level);
            if (facset == nullptr) {
                if (m_is_optional) {
                    return;
                }
                throw JException("Could not find parent at level=" + toString(m_level));
            }
            auto databundle = facset->GetDatabundle(std::type_index(typeid(PodioT)), m_databundle_name);
            if (databundle == nullptr) {
                if (!m_is_optional) {
                    facset->Print();
                    throw JException("Could not find databundle with type_index=" + JTypeInfo::demangle<PodioT>() + " and tag=" + m_databundle_name);
                }
                // data IS optional, so we exit
                m_data = nullptr;
                return;
            };
            if (databundle->GetFactory() != nullptr) {
                FactoryCreate(event, databundle->GetFactory());
            }
            auto* typed_databundle = dynamic_cast<JPodioDatabundle*>(databundle);
            if (typed_databundle == nullptr) {
                facset->Print();
                throw JException("Databundle with unique name '%s' does not inherit from JPodioDatabundle", m_databundle_name.c_str());
            }
            m_data = dynamic_cast<const typename PodioT::collection_type*>(typed_databundle->GetCollection());
            if (m_data == nullptr) {
                throw JException("Databundle with unique name '%s' does not contain %s", m_databundle_name.c_str(), JTypeInfo::demangle<typename PodioT::collection_type>().c_str());
            }
        }
    };
#endif


    template <typename T>
    class VariadicInput : public VariadicInputBase {

        std::vector<std::vector<const T*>> m_datas;

    public:

        VariadicInput(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(T));
            m_type_name = JTypeInfo::demangle<T>();
            m_level = JEventLevel::None;
        }

        VariadicInput(JHasInputs* owner, const VariadicInputOptions& options) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(T));
            m_type_name = JTypeInfo::demangle<T>();
            Configure(options);
        }

        void SetTags(std::vector<std::string> tags) {
            m_requested_databundle_names = tags;
        }

        const std::vector<std::vector<const T*>>& operator()() { return m_datas; }
        const std::vector<std::vector<const T*>>& operator*() { return m_datas; }
        const std::vector<std::vector<const T*>>* operator->() { return &m_datas; }

        const std::vector<const T*>& operator()(size_t index) { return m_datas.at(index); }


    private:
        friend class JComponentT;

        void Populate(const JEvent& event) {
            m_datas.clear();
            auto facset = GetFactorySetAtLevel(event, m_level);
            if (facset == nullptr) {
                if (m_is_optional) {
                    return;
                }
                throw JException("Could not find parent at level=" + toString(m_level));
            }
            if (!m_requested_databundle_names.empty()) {
                // We have a nonempty input, so we provide the user exactly the inputs they asked for (some of these may be null IF is_optional=true)
                for (auto& short_or_unique_name : m_requested_databundle_names) {
                    auto databundle = facset->GetDatabundle(std::type_index(typeid(T)), short_or_unique_name);
                    if (databundle == nullptr) {
                        if (!m_is_optional) {
                            facset->Print();
                            throw JException("Could not find databundle with type_index=" + JTypeInfo::demangle<T>() + " and name=" + short_or_unique_name);
                        }
                        m_datas.push_back({}); // If a databundle is optional and missing, we still insert an empty vector for it
                        continue;
                    };
                    if (databundle->GetFactory() != nullptr) {
                        FactoryCreate(event, databundle->GetFactory());
                    }
                    auto* typed_databundle = dynamic_cast<JLightweightDatabundleT<T>*>(databundle);
                    if (typed_databundle == nullptr) {
                        facset->Print();
                        throw JException("Databundle with shortname '%s' does not inherit from JLightweightDatabundleT<%s>", short_or_unique_name.c_str(), JTypeInfo::demangle<T>().c_str());
                    }
                    m_datas.push_back({});
                    auto& dest = m_datas.back();
                    dest.insert(dest.end(), typed_databundle->GetData().begin(), typed_databundle->GetData().end());
                }
            }
            else if (m_empty_input_policy == EmptyInputPolicy::IncludeEverything) {
                // We have an empty input and a nontrivial empty input policy
                m_realized_databundle_names.clear();

                auto databundles = facset->GetDatabundles(std::type_index(typeid(T)));
                for (auto* databundle : databundles) {

                    auto typed_databundle = dynamic_cast<JLightweightDatabundleT<T>*>(databundle);
                    if (typed_databundle == nullptr) {
                        throw JException("Databundle with name=" + typed_databundle->GetUniqueName() + " does not inherit from JLightweightDatabundleT<" + JTypeInfo::demangle<T>() + ">");
                    }
                    auto& contents = typed_databundle->GetData();
                    m_datas.push_back({}); // Create a destination for this factory's data
                    auto& dest = m_datas.back();
                    dest.insert(dest.end(), contents.begin(), contents.end());
                    if (databundle->HasShortName()) {
                        m_realized_databundle_names.push_back(databundle->GetShortName());
                    }
                    else {
                        m_realized_databundle_names.push_back(databundle->GetUniqueName());
                    }
                }
            }
        }
    };



#if JANA2_HAVE_PODIO
    template <typename PodioT>
    class VariadicPodioInput : public VariadicInputBase {

        std::vector<const typename PodioT::collection_type*> m_datas;

    public:

        VariadicPodioInput(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(PodioT));
            m_type_name = JTypeInfo::demangle<PodioT>();
        }

        VariadicPodioInput(JHasInputs* owner, const VariadicInputOptions& options) {
            owner->RegisterInput(this);
            m_type_index = std::type_index(typeid(PodioT));
            m_type_name = JTypeInfo::demangle<PodioT>();
            Configure(options);
        }

        const std::vector<const typename PodioT::collection_type*> operator()() {
            return m_datas;
        }

        void SetRequestedCollectionNames(std::vector<std::string> names) {
            m_requested_databundle_names = names;
            m_realized_databundle_names = std::move(names);
        }

        const std::vector<std::string>& GetRealizedCollectionNames() {
            return GetRealizedDatabundleNames();
        }

        void Populate(const JEvent& event) {
            auto facset = GetFactorySetAtLevel(event, m_level);
            if (facset == nullptr) {
                if (m_is_optional) {
                    return;
                }
                throw JException("Could not find parent at level=" + toString(m_level));
            }
            bool need_dynamic_realized_databundle_names = (m_requested_databundle_names.empty()) && (m_empty_input_policy != EmptyInputPolicy::IncludeNothing);
            if (need_dynamic_realized_databundle_names) {
                m_realized_databundle_names.clear();
            }
            m_datas.clear();
            if (!m_requested_databundle_names.empty()) {
                for (auto& short_or_unique_name : m_requested_databundle_names) {
                    auto databundle = facset->GetDatabundle(std::type_index(typeid(PodioT)), short_or_unique_name);
                    if (databundle == nullptr) {
                        if (!m_is_optional) {
                            facset->Print();
                            throw JException("Could not find databundle with type_index=" + JTypeInfo::demangle<PodioT>() + " and name=" + short_or_unique_name);
                        }
                        m_datas.push_back(nullptr);
                        continue;
                    }
                    if (databundle->GetFactory() != nullptr) {
                        FactoryCreate(event, databundle->GetFactory());
                    }
                    auto* typed_databundle = dynamic_cast<JPodioDatabundle*>(databundle);
                    if (typed_databundle == nullptr) {
                        facset->Print();
                        throw JException("Databundle with name '%s' does not inherit from JPodioDatabundle", short_or_unique_name.c_str());
                    }
                    auto* typed_collection = dynamic_cast<const typename PodioT::collection_type*>(typed_databundle->GetCollection());
                    if (typed_collection == nullptr) {
                        throw JException("Databundle with unique name '%s' does not contain %s", short_or_unique_name.c_str(), JTypeInfo::demangle<typename PodioT::collection_type>().c_str());
                    }

                    m_datas.push_back(typed_collection); // This MIGHT be nullptr if m_is_optional==true
                }
            }
            else if (m_empty_input_policy == EmptyInputPolicy::IncludeEverything) {
                auto databundles = facset->GetDatabundles(std::type_index(typeid(PodioT)));
                for (auto* databundle : databundles) {
                    if (databundle->GetFactory() != nullptr) {
                        FactoryCreate(event, databundle->GetFactory());
                    }
                    auto typed_databundle = dynamic_cast<JPodioDatabundle*>(databundle);
                    if (typed_databundle == nullptr) {
                        throw JException("Not a JPodioDatabundle: type_name=%s, unique_name=%s", databundle->GetTypeName().c_str(), databundle->GetUniqueName().c_str());
                    }
                    auto typed_collection = dynamic_cast<const typename PodioT::collection_type*>(typed_databundle->GetCollection());
                    if (typed_collection == nullptr) {
                        throw JException("Podio collection is not a %s: type_name=%s, unique_name=%s", JTypeInfo::demangle<PodioT>().c_str(), databundle->GetTypeName().c_str(), databundle->GetUniqueName().c_str());
                    }
                    m_datas.push_back(typed_collection);

                    if (databundle->HasShortName()) {
                        m_realized_databundle_names.push_back(databundle->GetShortName());
                    }
                    else {
                        m_realized_databundle_names.push_back(databundle->GetUniqueName());
                    }
                }
            }
        }
    };
#endif

    void WireInputs(JEventLevel component_level,
                    const std::vector<JEventLevel>& single_input_levels,
                    const std::vector<std::string>& single_input_databundle_names,
                    const std::vector<JEventLevel>& variadic_input_levels,
                    const std::vector<std::vector<std::string>>& variadic_input_databundle_names) {

        if (m_variadic_inputs.size() == 1 && variadic_input_databundle_names.size() == 0) {
            WireInputsCompatibility(component_level, single_input_levels, single_input_databundle_names);
            return;
        }

        // Validate that we have the correct number of input databundle names
        if (single_input_databundle_names.size() != m_inputs.size()) {
            throw JException("Wrong number of (nonvariadic) input databundle names! Expected %d, found %d", m_inputs.size(), single_input_databundle_names.size());
        }

        if (variadic_input_databundle_names.size() != m_variadic_inputs.size()) {
            throw JException("Wrong number of variadic input databundle names! Expected %d, found %d", m_variadic_inputs.size(), variadic_input_databundle_names.size());
        }

        size_t i = 0;
        for (auto* input : m_inputs) {
            input->SetDatabundleName(single_input_databundle_names.at(i));
            if (single_input_levels.empty()) {
                input->SetLevel(component_level);
            }
            else {
                input->SetLevel(single_input_levels.at(i));
            }
            i += 1;
        }

        i = 0;
        for (auto* variadic_input : m_variadic_inputs) {
            variadic_input->SetRequestedDatabundleNames(variadic_input_databundle_names.at(i));
            if (variadic_input_levels.empty()) {
                variadic_input->SetLevel(component_level);
            }
            else {
                variadic_input->SetLevel(variadic_input_levels.at(i));
            }
            i += 1;
        }
    }

    void WireInputsCompatibility(JEventLevel component_level,
                    const std::vector<JEventLevel>& single_input_levels,
                    const std::vector<std::string>& single_input_databundle_names) {

        // Figure out how many collection names belong to the variadic input
        int variadic_databundle_count = single_input_databundle_names.size() - m_inputs.size();
        int databundle_name_index = 0;
        int databundle_level_index = 0;

        for (auto& pair : m_ordered_inputs) {
            auto* single_input = pair.first;
            auto* variadic_input = pair.second;
            if (single_input != nullptr) {
                single_input->SetDatabundleName(single_input_databundle_names.at(databundle_name_index));
                if (single_input_levels.empty()) {
                    single_input->SetLevel(component_level);
                }
                else {
                    single_input->SetLevel(single_input_levels.at(databundle_level_index));
                }
                databundle_name_index += 1;
                databundle_level_index += 1;
            }
            else {
                std::vector<std::string> variadic_databundle_names;
                for (int i=0; i<variadic_databundle_count; ++i) {
                    variadic_databundle_names.push_back(single_input_databundle_names.at(databundle_name_index+i));
                }
                variadic_input->SetRequestedDatabundleNames(variadic_databundle_names);
                if (single_input_levels.empty()) {
                    variadic_input->SetLevel(component_level);
                }
                else {
                    variadic_input->SetLevel(single_input_levels.at(databundle_level_index)); // Last one wins!
                }
                databundle_name_index += variadic_databundle_count;
                databundle_level_index += 1;
            }
        }
    }

    void SummarizeInputs(JComponentSummary::Component& summary) const {
        for (const auto* input : m_inputs) {
            summary.AddInput(new JComponentSummary::Collection("", input->GetDatabundleName(), input->GetTypeName(), input->GetLevel()));
        }
        for (const auto* input : m_variadic_inputs) {
            for (auto& databundle_name : input->GetRequestedDatabundleNames()) {
                summary.AddInput(new JComponentSummary::Collection("", databundle_name, input->GetTypeName(), input->GetLevel()));
            }
        }
    }
};

} // namespace jana::components

