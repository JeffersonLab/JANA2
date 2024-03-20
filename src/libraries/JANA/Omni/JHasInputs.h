// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/JEvent.h>

namespace jana {
namespace omni {

struct JHasInputs {
protected:

    struct InputBase;
    std::vector<InputBase*> m_inputs;

    void RegisterInput(InputBase* input) {
        m_inputs.push_back(input);
    }

    struct InputBase {
        std::string type_name;
        JEventLevel level;
        std::vector<std::string> collection_names;
        bool is_variadic = false;

        virtual void GetCollection(const JEvent& event) = 0;
    };

    template <typename T, typename ComponentT>
    class Input : public InputBase {

        std::vector<const T*> m_data;

    public:
        Input(ComponentT* owner, JEventLevel level=JEventLevel::Event, std::string default_tag="") {
            owner->RegisterInput(this);
            this->collection_names.push_back(default_tag);
            this->type_name = JTypeInfo::demangle<T>();
            this->level = level;
        }

        const std::vector<const T*>& operator()() { return m_data; }

    private:
        friend class JComponentT;

        void GetCollection(const JEvent& event) {
            if (this->level == event.GetLevel()) {
                m_data = event.Get<T>(this->collection_names[0]);
            }
            else {
                m_data = event.GetParent(level).template Get<T>(this->collection_names[0]);
            }
        }
    };

#ifdef JANA2_HAVE_PODIO
    template <typename PodioT>
    class PodioInput : public InputBase {

        const typename PodioTypeMap<PodioT>::collection_t* m_data;

    public:

        PodioInput(JOmniFactory* owner, JEventLevel level=JEventLevel::Event, std::string default_collection_name="") {
            owner->RegisterInput(this);
            this->collection_names.push_back(default_collection_name);
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->level = level;
        }

        const typename PodioTypeMap<PodioT>::collection_t* operator()() {
            return m_data;
        }

    private:
        friend class JOmniFactory;

        void GetCollection(const JEvent& event) {
            if (this->level == event->GetLevel()) {
                m_data = event.GetCollection<PodioT>(this->collection_names[0]);
            }
            else {
                m_data = event.GetParent(this->level).GetCollection<PodioT>(this->collection_names[0]);
            }
        }
    };


    template <typename PodioT>
    class VariadicPodioInput : public InputBase {

        std::vector<const typename PodioTypeMap<PodioT>::collection_t*> m_data;

    public:

        VariadicPodioInput(JOmniFactory* owner, JEventLevel level=JEventLevel::Event, std::vector<std::string> default_names = {}) {
            owner->RegisterInput(this);
            this->collection_names = default_names;
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->is_variadic = true;
            this->level = level;
        }

        const std::vector<const typename PodioTypeMap<PodioT>::collection_t*> operator()() {
            return m_data;
        }

    private:
        friend class JOmniFactory;

        void GetCollection(const JEvent& event) {
            m_data.clear();
            if (this->level == event->GetLevel()) {
                for (auto& coll_name : this->collection_names) {
                    m_data.push_back(event.GetCollection<PodioT>(coll_name));
                }
            }
            else {
                for (auto& coll_name : this->collection_names) {
                    m_data.push_back(event.GetParent(this->level).GetCollection<PodioT>(this->collection_names[0]));
                }
            }
        }
    };
#endif
};

} // namespace omni
} // namespace jana

