// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include "JANA/Podio/JFactoryPodioT.h"
#include <JANA/JEvent.h>


namespace jana::components {

struct JHasInputs {
protected:

    class InputBase;
    class VariadicInputBase;

    std::vector<InputBase*> m_inputs;
    std::vector<VariadicInputBase*> m_variadic_inputs;

    void RegisterInput(InputBase* input) {
        m_inputs.push_back(input);
    }

    void RegisterInput(VariadicInputBase* input) {
        m_variadic_inputs.push_back(input);
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
        std::string m_type_name;
        std::string m_databundle_name;
        JEventLevel m_level = JEventLevel::None;
        bool m_is_optional = false;

    public:

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

        virtual void GetCollection(const JEvent& event) = 0;
        virtual void PrefetchCollection(const JEvent& event) = 0;
    };

    class VariadicInputBase {
    public:
        enum class EmptyInputPolicy { IncludeNothing, IncludeEverything };

    protected:
        std::string m_type_name;
        std::vector<std::string> m_requested_databundle_names;
        std::vector<std::string> m_realized_databundle_names;
        JEventLevel m_level = JEventLevel::None;
        bool m_is_optional = false;
        EmptyInputPolicy m_empty_input_policy = EmptyInputPolicy::IncludeNothing;

    public:

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

        virtual void GetCollection(const JEvent& event) = 0;
        virtual void PrefetchCollection(const JEvent& event) = 0;
    };


    template <typename T>
    class Input : public InputBase {

        std::vector<const T*> m_data;
        std::string m_tag;

    public:

        Input(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_name = JTypeInfo::demangle<T>();
            m_databundle_name = m_type_name;
            m_level = JEventLevel::None;
        }

        Input(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
            m_type_name = JTypeInfo::demangle<T>();
            Configure(options);
        }

        void SetTag(std::string tag) {
            m_tag = tag;
            m_databundle_name = m_type_name + ":" + tag;
        }

        const std::vector<const T*>& operator()() { return m_data; }
        const std::vector<const T*>& operator*() { return m_data; }
        const std::vector<const T*>* operator->() { return &m_data; }


    private:
        friend class JComponentT;

        void GetCollection(const JEvent& event) {
            auto& level = m_level;
            m_data.clear();
            if (level == event.GetLevel() || level == JEventLevel::None) {
                event.Get<T>(m_data, m_tag, !m_is_optional);
            }
            else {
                if (m_is_optional && !event.HasParent(level)) return;
                event.GetParent(level).template Get<T>(m_data, m_tag, !m_is_optional);
            }
        }
        void PrefetchCollection(const JEvent& event) {
            if (m_level == event.GetLevel() || m_level == JEventLevel::None) {
                event.GetFactory<T>(m_tag, !m_is_optional)->Create(event.shared_from_this());
            }
            else {
                if (m_is_optional && !event.HasParent(m_level)) return;
                event.GetParent(m_level).template GetFactory<T>(m_tag, !m_is_optional)->Create(event.shared_from_this());
            }
        }
    };

#if JANA2_HAVE_PODIO
    template <typename PodioT>
    class PodioInput : public InputBase {

        const typename PodioT::collection_type* m_data;

    public:

        PodioInput(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_name = JTypeInfo::demangle<PodioT>();
            m_databundle_name = m_type_name;
            m_level = JEventLevel::None;
        }

        PodioInput(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
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

        void GetCollection(const JEvent& event) {
            if (m_level == event.GetLevel() || m_level == JEventLevel::None) {
                m_data = event.GetCollection<PodioT>(m_databundle_name, !m_is_optional);
            }
            else {
                if (m_is_optional && !event.HasParent(m_level)) return;
                m_data = event.GetParent(m_level).template GetCollection<PodioT>(m_databundle_name, !m_is_optional);
            }
        }

        void PrefetchCollection(const JEvent& event) {
            if (m_level == event.GetLevel() || m_level == JEventLevel::None) {
                event.GetCollection<PodioT>(m_databundle_name, !m_is_optional);
            }
            else {
                if (m_is_optional && !event.HasParent(m_level)) return;
                event.GetParent(m_level).template GetCollection<PodioT>(m_databundle_name, !m_is_optional);
            }
        }
    };


    template <typename T>
    class VariadicInput : public VariadicInputBase {

        std::vector<std::vector<const T*>> m_datas;
        std::vector<std::string> m_tags;

    public:

        VariadicInput(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_name = JTypeInfo::demangle<T>();
            m_level = JEventLevel::None;
        }

        VariadicInput(JHasInputs* owner, const VariadicInputOptions& options) {
            owner->RegisterInput(this);
            m_type_name = JTypeInfo::demangle<T>();
            Configure(options);
        }

        void SetTags(std::vector<std::string> tags) {
            m_tags = std::move(tags);
            m_requested_databundle_names.clear();
            for (auto& tag : tags) {
                m_requested_databundle_names.push_back(m_type_name + ":" + tag);
            }
        }

        const std::vector<std::vector<const T*>>& operator()() { return m_datas; }
        const std::vector<std::vector<const T*>>& operator*() { return m_datas; }
        const std::vector<std::vector<const T*>>* operator->() { return &m_datas; }

        const std::vector<const T*>& operator()(size_t index) { return m_datas.at(index); }


    private:
        friend class JComponentT;

        void GetCollection(const JEvent& event) {
            m_datas.clear();
            if (m_level == event.GetLevel() || m_level == JEventLevel::None) {
                size_t i=0;
                for (auto& tag : m_tags) {
                    m_datas.push_back({});
                    event.Get<T>(m_datas.at(i++), tag, !m_is_optional);
                }
            }
            else {
                if (m_is_optional && !event.HasParent(m_level)) return;
                auto& parent = event.GetParent(m_level);
                size_t i=0;
                for (auto& tag : m_tags) {
                    m_datas.push_back({});
                    parent.template Get<T>(m_datas.at(i++), tag, !m_is_optional);
                }
            }
        }
        void PrefetchCollection(const JEvent& event) {
            if (m_level == event.GetLevel() || m_level == JEventLevel::None) {
                for (auto& tag : m_tags) {
                    event.GetFactory<T>(tag, !m_is_optional)->Create(event.shared_from_this());
                }
            }
            else {
                if (m_is_optional && !event.HasParent(m_level)) return;
                auto& parent = event.GetParent(m_level);
                for (auto& tag : m_tags) {
                    parent.template GetFactory<T>(tag, !m_is_optional)->Create(event.shared_from_this());
                }
            }
        }
    };



    template <typename PodioT>
    class VariadicPodioInput : public VariadicInputBase {

        std::vector<const typename PodioT::collection_type*> m_datas;

    public:

        VariadicPodioInput(JHasInputs* owner) {
            owner->RegisterInput(this);
            m_type_name = JTypeInfo::demangle<PodioT>();
        }

        VariadicPodioInput(JHasInputs* owner, const VariadicInputOptions& options) {
            owner->RegisterInput(this);
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

        void GetCollection(const JEvent& event) {
            bool need_dynamic_realized_databundle_names = (m_requested_databundle_names.empty()) && (m_empty_input_policy != EmptyInputPolicy::IncludeNothing);
            if (need_dynamic_realized_databundle_names) {
                m_realized_databundle_names.clear();
            }
            m_datas.clear();
            if (!m_requested_databundle_names.empty()) {
                for (auto& coll_name : m_requested_databundle_names) {
                    if (m_level == event.GetLevel() || m_level == JEventLevel::None) {
                        auto coll = event.GetCollection<PodioT>(coll_name, !m_is_optional);
                        m_datas.push_back(coll);
                    }
                    else {
                        if (m_is_optional && !event.HasParent(m_level)) return;
                        auto coll = event.GetParent(m_level).template GetCollection<PodioT>(coll_name, !m_is_optional);
                        m_datas.push_back(coll);
                    }
                }
            }
            else if (m_empty_input_policy == EmptyInputPolicy::IncludeEverything) {
                auto facs = event.GetFactorySet()->GetAllFactories<PodioT>();
                for (auto* fac : facs) {
                    JFactoryPodioT<PodioT>* podio_fac = dynamic_cast<JFactoryPodioT<PodioT>*>(fac);
                    if (podio_fac == nullptr) {
                        throw JException("Found factory which is NOT a podio factory!");
                    }
                    auto typed_collection = dynamic_cast<const PodioT::collection_type*>(podio_fac->GetCollection());
                    m_datas.push_back(typed_collection);
                    if (need_dynamic_realized_databundle_names) {
                        m_realized_databundle_names.push_back(podio_fac->GetTag());
                    }
                }
            }
        }

        void PrefetchCollection(const JEvent& event) {
            if (!m_requested_databundle_names.empty()) {
                for (auto& coll_name : m_requested_databundle_names) {
                    if (m_level == event.GetLevel() || m_level == JEventLevel::None) {
                        event.GetCollection<PodioT>(coll_name, !m_is_optional);
                    }
                    else {
                        if (m_is_optional && !event.HasParent(m_level)) return;
                        event.GetParent(m_level).template GetCollection<PodioT>(coll_name, !m_is_optional);
                    }
                }
            }
            else if (m_empty_input_policy == EmptyInputPolicy::IncludeEverything) {
                auto facs = event.GetFactorySet()->GetAllFactories<PodioT>();
                for (auto* fac : facs) {
                    fac->Create(event.shared_from_this());
                }
            }
        }
    };
#endif
};

} // namespace jana::components

