// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include <JEvent.h>

namespace jana {
namespace omni {


struct JHasOutputs {
protected:
    class OutputBase;
    std::vector<OutputBase*> m_outputs;

    void RegisterOutput(OutputBase* output) {
        m_outputs.push_back(output);
    }

    struct OutputBase {
        JEventLevel level = JEventLevel::Event;
        std::string type_name;
        std::vector<std::string> collection_names;
        bool is_variadic = false;

        virtual void InsertCollection(JEvent& event) = 0;
        virtual void Reset() = 0;
    };

    template <typename T>
    class Output : public OutputBase {
        std::vector<T*> m_data;

    public:
        Output(JHasOutput* owner, std::string default_tag_name="") {
            owner->RegisterOutput(this);
            this->collection_names.push_back(default_tag_name);
            this->type_name = JTypeInfo::demangle<T>();
        }

        std::vector<T*>& operator()() { return m_data; }

    private:
        void InsertCollection(JEvent& event) override {
            event->Insert(m_data, this->collection_names[0]);
        }
        void Reset() override { }
    };


#ifdef JANA2_HAVE_PODIO
    template <typename PodioT>
    class PodioOutput : public EventOutputBase {

        std::unique_ptr<typename PodioTypeMap<PodioT>::collection_t> m_data;

    public:

        PodioOutput(JHasOutput* owner, std::string default_collection_name="") {
            owner->RegisterOutput(this);
            this->collection_names.push_back(default_collection_name);
            this->type_name = JTypeInfo::demangle<PodioT>();
        }

        std::unique_ptr<typename PodioTypeMap<PodioT>::collection_t>& operator()() { return m_data; }

    private:
        void InsertCollection(JEvent& event) override {
            event->InsertCollection(std::move(*m_data), this->collection_names[0]);
        }
        void Reset() override {
            m_data = std::move(std::make_unique<typename PodioTypeMap<PodioT>::collection_t>());
        }
    };


    template <typename PodioT>
    class VariadicPodioOutput : public EventOutputBase {

        std::vector<std::unique_ptr<typename PodioTypeMap<PodioT>::collection_t>> m_data;

    public:

        VariadicPodioOutput(JHasOutput* owner, std::vector<std::string> default_collection_names={}) {
            owner->RegisterOutput(this);
            this->collection_names = default_collection_names;
            this->type_name = JTypeInfo::demangle<PodioT>();
            this->is_variadic = true;
        }

        std::vector<std::unique_ptr<typename PodioTypeMap<PodioT>::collection_t>>& operator()() { return m_data; }

    private:
        void InsertCollection(JEvent& event) override {
            if (m_data.size() != this->collection_names.size()) {
                throw JException("JOmniFactory: VariadicPodioOutput InsertCollection failed: Declared %d collections, but provided %d.", this->collection_names.size(), m_data.size());
                // Otherwise this leads to a PODIO segfault
            }
            size_t i = 0;
            for (auto& coll_name : this->collection_names) {
                event->InsertCollection(std::move(*(m_data[i++])), coll_name);
            }
        }

        void Reset() override {
            m_data.clear();
            for (auto& coll_name : this->collection_names) {
                m_data.push_back(std::make_unique<typename PodioTypeMap<PodioT>::collection_t>());
            }
        }
    };
#endif

}



} // namespace omni
} // namespace jana
