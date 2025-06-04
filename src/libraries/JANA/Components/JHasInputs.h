// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/JEvent.h>


namespace jana::components {

struct JHasInputs {
protected:

    struct InputBase;
    struct VariadicInputBase;

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

    struct InputBase {
        std::string type_name;
        std::string databundle_name;
        JEventLevel level = JEventLevel::None;
        bool is_optional = false;

        void SetOptional(bool isOptional) {
            this->is_optional = isOptional;
        }

        void SetLevel(JEventLevel level) {
            this->level = level;
        }

        void SetDatabundleName(std::string name) {
            this->databundle_name = name;
        }

        const std::string& GetDatabundleName() const {
            return databundle_name;
        }

        void Configure(const InputOptions& options) {
            this->databundle_name = options.name;
            this->level = options.level;
            this->is_optional = options.is_optional;
            // this->is_shortcircuiting = options.is_shortcircuiting;
            // this->contains_single_item = options.contains_single_item;
        }

        virtual void GetCollection(const JEvent& event) = 0;
        virtual void PrefetchCollection(const JEvent& event) = 0;
    };

    struct VariadicInputBase {
        std::string type_name;
        std::vector<std::string> databundle_names;
        JEventLevel level = JEventLevel::None;
        bool is_optional = false;

        void SetOptional(bool isOptional) {
            this->is_optional = isOptional;
        }

        void SetLevel(JEventLevel level) {
            this->level = level;
        }

        void SetDatabundleNames(std::vector<std::string> names) {
            this->databundle_names = names;
        }

        const std::vector<std::string>& GetDatabundleNames() const {
            return databundle_names;
        }

        void Configure(const VariadicInputOptions& options) {
            this->databundle_names = options.names;
            this->level = options.level;
            this->is_optional = options.is_optional;
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
            this->type_name = JTypeInfo::demangle<T>();
            this->databundle_name = type_name;
            // For non-PODIO inputs, these are technically tags for now, not names
            this->level = JEventLevel::None;
        }

        Input(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<T>();
            Configure(options);
        }

        void SetTag(std::string tag) {
            m_tag = tag;
            databundle_name = type_name + ":" + tag;
        }

        const std::vector<const T*>& operator()() { return m_data; }
        const std::vector<const T*>& operator*() { return m_data; }
        const std::vector<const T*>* operator->() { return &m_data; }


    private:
        friend class JComponentT;

        void GetCollection(const JEvent& event) {
            auto& level = this->level;
            m_data.clear();
            if (level == event.GetLevel() || level == JEventLevel::None) {
                event.Get<T>(m_data, m_tag, !this->is_optional);
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                event.GetParent(level).template Get<T>(m_data, m_tag, !this->is_optional);
            }
        }
        void PrefetchCollection(const JEvent& event) {
            if (level == event.GetLevel() || level == JEventLevel::None) {
                event.GetFactory<T>(m_tag, !this->is_optional)->Create(event.shared_from_this());
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                event.GetParent(level).template GetFactory<T>(m_tag, !this->is_optional)->Create(event.shared_from_this());
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
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->databundle_name = this->type_name;
            this->level = JEventLevel::None;
        }

        PodioInput(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->databundle_name = this->type_name;
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
            this->databundle_name = name;
        }

        void SetTag(std::string tag) {
            this->databundle_name = type_name + ":" + tag;
        }

        void GetCollection(const JEvent& event) {
            if (level == event.GetLevel() || level == JEventLevel::None) {
                m_data = event.GetCollection<PodioT>(databundle_name, !this->is_optional);
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                m_data = event.GetParent(level).template GetCollection<PodioT>(databundle_name, !this->is_optional);
            }
        }

        void PrefetchCollection(const JEvent& event) {
            if (level == event.GetLevel() || level == JEventLevel::None) {
                event.GetCollection<PodioT>(databundle_name, !this->is_optional);
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                event.GetParent(level).template GetCollection<PodioT>(databundle_name, !this->is_optional);
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
            this->type_name = JTypeInfo::demangle<T>();
            this->level = JEventLevel::None;
        }

        VariadicInput(JHasInputs* owner, const VariadicInputOptions& options) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<T>();
            Configure(options);
        }

        void SetTags(std::vector<std::string> tags) {
            this->m_tags = std::move(tags);
            this->databundle_names.clear();
            for (auto& tag : tags) {
                this->databundle_names.push_back(type_name + ":" + tag);
            }
        }

        const std::vector<std::vector<const T*>>& operator()() { return m_datas; }
        const std::vector<std::vector<const T*>>& operator*() { return m_datas; }
        const std::vector<std::vector<const T*>>* operator->() { return &m_datas; }

        const std::vector<const T*>& operator()(size_t index) { return m_datas.at(index); }


    private:
        friend class JComponentT;

        void GetCollection(const JEvent& event) {
            auto& level = this->level;
            m_datas.clear();
            if (level == event.GetLevel() || level == JEventLevel::None) {
                size_t i=0;
                for (auto& tag : this->m_tags) {
                    event.Get<T>(m_datas.at(i++), tag, !this->is_optional);
                }
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                auto& parent = event.GetParent(level);
                size_t i=0;
                for (auto& tag : this->m_tags) {
                    parent.template Get<T>(m_datas.at(i++), tag, !this->is_optional);
                }
            }
        }
        void PrefetchCollection(const JEvent& event) {
            if (level == event.GetLevel() || level == JEventLevel::None) {
                for (auto& tag : this->m_tags) {
                    event.GetFactory<T>(tag, !this->is_optional)->Create(event.shared_from_this());
                }
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                auto& parent = event.GetParent(level);
                for (auto& tag : this->m_tags) {
                    parent.template GetFactory<T>(tag, !this->is_optional)->Create(event.shared_from_this());
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
            this->type_name = JTypeInfo::demangle<PodioT>();
        }

        VariadicPodioInput(JHasInputs* owner, const VariadicInputOptions& options) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<PodioT>();
            Configure(options);
        }

        const std::vector<const typename PodioT::collection_type*> operator()() {
            return m_datas;
        }

        void SetCollectionNames(std::vector<std::string> names) {
            this->databundle_names = std::move(names);
        }

        void GetCollection(const JEvent& event) {
            m_datas.clear();
            for (auto& coll_name : databundle_names) {
                if (level == event.GetLevel() || level == JEventLevel::None) {
                    m_datas.push_back(event.GetCollection<PodioT>(coll_name, !this->is_optional));
                }
                else {
                    if (this->is_optional && !event.HasParent(level)) return;
                    m_datas.push_back(event.GetParent(level).template GetCollection<PodioT>(coll_name, !this->is_optional));
                }
            }
        }

        void PrefetchCollection(const JEvent& event) {
            for (auto& coll_name : databundle_names) {
                if (level == event.GetLevel() || level == JEventLevel::None) {
                    event.GetCollection<PodioT>(coll_name, !this->is_optional);
                }
                else {
                    if (this->is_optional && !event.HasParent(level)) return;
                    event.GetParent(level).template GetCollection<PodioT>(coll_name, !this->is_optional);
                }
            }
        }
    };
#endif
};

} // namespace jana::components

