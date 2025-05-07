// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include <JANA/JEvent.h>

namespace jana::components {



struct JHasOutputs {
protected:
    struct OutputBase;
    std::vector<OutputBase*> m_outputs;

    void RegisterOutput(OutputBase* output) {
        m_outputs.push_back(output);
    }

    struct OutputBase {
        std::string type_name;
        std::vector<std::string> collection_names;
        bool is_variadic = false;

        virtual void InsertCollection(JEvent& event) = 0;
        virtual void Reset() = 0;
    };

    template <typename T>
    class Output : public OutputBase {
        std::vector<T*> m_data;
        bool is_not_owner = false;

    public:
        Output(JHasOutputs* owner, std::string default_tag_name="") {
            owner->RegisterOutput(this);
            this->collection_names.push_back(default_tag_name);
            this->type_name = JTypeInfo::demangle<T>();
        }

        std::vector<T*>& operator()() { return m_data; }

    protected:
        void InsertCollection(JEvent& event) override {
            auto fac = event.Insert(m_data, this->collection_names[0]);
            fac->SetIsNotOwnerFlag(is_not_owner);
        }
        void Reset() override { }

        void SetIsNotOwnerFlag(bool not_owner=true) { is_not_owner = not_owner; }
    };


#if JANA2_HAVE_PODIO
    template <typename PodioT>
    class PodioOutput : public OutputBase {

        std::unique_ptr<typename PodioT::collection_type> m_data;

    public:

        PodioOutput(JHasOutputs* owner, std::string default_collection_name="") {
            owner->RegisterOutput(this);
            this->collection_names.push_back(default_collection_name);
            this->type_name = JTypeInfo::demangle<PodioT>();
        }

        std::unique_ptr<typename PodioT::collection_type>& operator()() { return m_data; }

    protected:
        void InsertCollection(JEvent& event) override {
            event.InsertCollection<PodioT>(std::move(*m_data), this->collection_names[0]);
        }
        void Reset() override {
            m_data = std::move(std::make_unique<typename PodioT::collection_type>());
        }
    };


    template <typename PodioT>
    class VariadicPodioOutput : public OutputBase {

        std::vector<std::unique_ptr<typename PodioT::collection_type>> m_data;

    public:

        VariadicPodioOutput(JHasOutputs* owner, std::vector<std::string> default_collection_names={}) {
            owner->RegisterOutput(this);
            this->collection_names = default_collection_names;
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->is_variadic = true;
        }

        std::vector<std::unique_ptr<typename PodioT::collection_type>>& operator()() { return m_data; }

    protected:
        void InsertCollection(JEvent& event) override {
            if (m_data.size() != this->collection_names.size()) {
                throw JException("VariadicPodioOutput InsertCollection failed: Declared %d collections, but provided %d.", this->collection_names.size(), m_data.size());
                // Otherwise this leads to a PODIO segfault
            }
            size_t i = 0;
            for (auto& coll_name : this->collection_names) {
                event.InsertCollection<PodioT>(std::move(*(m_data[i++])), coll_name);
            }
        }

        void Reset() override {
            m_data.clear();
            for (auto& coll_name : this->collection_names) {
                m_data.push_back(std::make_unique<typename PodioT::collection_type>());
            }
        }
    };
#endif

};


} // namespace jana::components
