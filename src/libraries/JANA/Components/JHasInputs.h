// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/JEvent.h>


namespace jana::components {

struct JHasInputs {
protected:

    struct InputBase;
    std::vector<InputBase*> m_inputs;

    void RegisterInput(InputBase* input) {
        m_inputs.push_back(input);
    }
    
    struct InputOptions {
        std::string name {""};
        JEventLevel level {JEventLevel::None};
        bool is_optional {false};
        // bool is_shortcircuiting {false};
        // bool contains_single_item {false};
    };

    struct VariadicInputOptions {
        std::vector<std::string> names {""};
        JEventLevel level {JEventLevel::None};
        bool is_optional {false};
        // bool is_shortcircuiting {false};
        // bool contains_single_item {false};
    };

    struct InputBase {
        std::string type_name;
        std::vector<std::string> names;
        JEventLevel level;
        bool is_variadic = false;
        bool is_optional = false;
        //bool is_shortcircuiting = false;
        //bool contains_single_item = false;


        void Configure(const InputOptions& options) {
            this->names.clear();
            this->names.push_back(options.name);
            this->level = options.level;
            this->is_optional = options.is_optional;
            // this->is_shortcircuiting = options.is_shortcircuiting;
            // this->contains_single_item = options.contains_single_item;
        }

        void ConfigureVariadic(const VariadicInputOptions& options) {
            if (!is_variadic) { throw JException("Setting variadic options on non-variadic input"); }
            this->names = options.names;
            this->level = options.level;
            this->is_optional = options.is_optional;
            // this->is_shortcircuiting = options.is_shortcircuiting;
            // this->contains_single_item = options.contains_single_item;
        }

        virtual void GetCollection(const JEvent& event) = 0;
        virtual void PrefetchCollection(const JEvent& event) = 0;
    };

    template <typename T>
    class Input : public InputBase {

        std::vector<const T*> m_data;

    public:

        Input(JHasInputs* owner) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<T>();
            this->names.push_back("");
            // For non-PODIO inputs, these are technically tags for now, not names
            this->level = JEventLevel::None;
        }

        Input(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<T>();
            Configure(options);
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
                event.Get<T>(m_data, this->names[0], !this->is_optional);
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                event.GetParent(level).template Get<T>(m_data, this->names[0], !this->is_optional);
            }
        }
        void PrefetchCollection(const JEvent& event) {
            auto& level = this->level;
            auto& name = this->names[0];
            if (level == event.GetLevel() || level == JEventLevel::None) {
                event.GetFactory<T>(name, !this->is_optional)->Create(event.shared_from_this());
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                event.GetParent(level).template GetFactory<T>(name, !this->is_optional)->Create(event.shared_from_this());
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
            this->names.push_back(this->type_name);
            this->level = JEventLevel::None;
        }

        PodioInput(JHasInputs* owner, const InputOptions& options) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<PodioT>();
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

        void GetCollection(const JEvent& event) {
            auto& level = this->level;
            auto& name = this->names[0];
            if (level == event.GetLevel() || level == JEventLevel::None) {
                m_data = event.GetCollection<PodioT>(name, !this->is_optional);
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                m_data = event.GetParent(level).template GetCollection<PodioT>(name, !this->is_optional);
            }
        }

        void PrefetchCollection(const JEvent& event) {
            auto& level = this->level;
            auto& name = this->names[0];
            if (level == event.GetLevel() || level == JEventLevel::None) {
                event.GetCollection<PodioT>(name, !this->is_optional);
            }
            else {
                if (this->is_optional && !event.HasParent(level)) return;
                event.GetParent(level).template GetCollection<PodioT>(name, !this->is_optional);
            }
        }
    };


    template <typename PodioT>
    class VariadicPodioInput : public InputBase {

        std::vector<const typename PodioT::collection_type*> m_data;

    public:

        VariadicPodioInput(JHasInputs* owner) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->is_variadic = true;
        }

        VariadicPodioInput(JHasInputs* owner, const VariadicInputOptions& options) {
            owner->RegisterInput(this);
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->is_variadic = true;
            ConfigureVariadic(options);
        }

        const std::vector<const typename PodioT::collection_type*> operator()() {
            return m_data;
        }

        void GetCollection(const JEvent& event) {
            m_data.clear();
            for (auto& coll_name : names) {
                if (level == event.GetLevel() || level == JEventLevel::None) {
                    m_data.push_back(event.GetCollection<PodioT>(coll_name, !this->is_optional));
                }
                else {
                    if (this->is_optional && !event.HasParent(level)) return;
                    m_data.push_back(event.GetParent(level).template GetCollection<PodioT>(coll_name, !this->is_optional));
                }
            }
        }

        void PrefetchCollection(const JEvent& event) {
            for (auto& coll_name : names) {
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

