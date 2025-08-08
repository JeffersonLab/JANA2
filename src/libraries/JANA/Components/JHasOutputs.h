// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include "JANA/Components/JComponentSummary.h"
#include "JANA/Utils/JEventLevel.h"
#include <JANA/JEvent.h>
#include <JANA/JMultifactory.h>

namespace jana::components {



struct JHasOutputs {
public:
    struct OutputBase;

protected:
    std::vector<OutputBase*> m_outputs;

public:
    void RegisterOutput(OutputBase* output) {
        m_outputs.push_back(output);
    }

    struct OutputBase {
        std::string type_name;
        std::vector<std::string> collection_names;
        JEventLevel level = JEventLevel::None;
        bool is_variadic = false;

        virtual void CreateHelperFactory(JMultifactory& fac) = 0;
        virtual void SetCollection(JMultifactory& fac) = 0;
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

        void SetTag(std::string tag) {
            this->collection_names.clear();
            this->collection_names.push_back(tag);
        }

        std::vector<T*>& operator()() { return m_data; }

        void SetNotOwnerFlag(bool not_owner=true) { is_not_owner = not_owner; }

    protected:

        void CreateHelperFactory(JMultifactory& fac) override {
            fac.DeclareOutput<T>(this->collection_names[0]);
        }

        void SetCollection(JMultifactory& fac) override {
            fac.SetData<T>(this->collection_names[0], this->m_data);
        }

        void InsertCollection(JEvent& event) override {
            auto fac = event.Insert(m_data, this->collection_names[0]);
            fac->SetNotOwnerFlag(is_not_owner);
        }
        void Reset() override { 
            m_data.clear();
        }

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

        void SetCollectionName(std::string name) {
            this->collection_names.clear();
            this->collection_names.push_back(name);
        }

    protected:

        void CreateHelperFactory(JMultifactory& fac) override {
            fac.DeclarePodioOutput<PodioT>(this->collection_names[0]);
        }

        void SetCollection(JMultifactory& fac) override {
            if (m_data == nullptr) {
                throw JException("JOmniFactory: SetCollection failed due to missing output collection '%s'", this->collection_names[0].c_str());
                // Otherwise this leads to a PODIO segfault
            }
            fac.SetCollection<PodioT>(this->collection_names[0], std::move(this->m_data));
        }

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

        void SetCollectionNames(std::vector<std::string> names) {
            this->collection_names = names;
        }

    protected:

        void CreateHelperFactory(JMultifactory& fac) override {
            for (auto& coll_name : this->collection_names) {
                fac.DeclarePodioOutput<PodioT>(coll_name);
            }
        }

        void SetCollection(JMultifactory& fac) override {
            if (m_data.size() != this->collection_names.size()) {
                throw JException("JOmniFactory: VariadicPodioOutput SetCollection failed: Declared %d collections, but provided %d.", this->collection_names.size(), m_data.size());
                // Otherwise this leads to a PODIO segfault
            }
            size_t i = 0;
            for (auto& coll_name : this->collection_names) {
                fac.SetCollection<PodioT>(coll_name, std::move(this->m_data[i++]));
            }
        }

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
            for (size_t i=0; i<collection_names.size(); ++i) {
                m_data.push_back(std::make_unique<typename PodioT::collection_type>());
            }
        }
    };
#endif


    void WireOutputs(JEventLevel component_level, const std::vector<std::string>& single_output_databundle_names, const std::vector<std::vector<std::string>>& variadic_output_databundle_names) {
        size_t single_output_index = 0;
        size_t variadic_output_index = 0;

        for (auto* output : m_outputs) {
            output->collection_names.clear();
            output->level = component_level;
            if (output->is_variadic) {
                output->collection_names = variadic_output_databundle_names.at(variadic_output_index++);
            }
            else {
                output->collection_names.push_back(single_output_databundle_names.at(single_output_index++));
            }
        }
    }

    void SummarizeOutputs(JComponentSummary::Component& summary) const {
        for (const auto* output : m_outputs) {
            size_t suboutput_count = output->collection_names.size();
            for (size_t i=0; i<suboutput_count; ++i) {
                summary.AddOutput(new JComponentSummary::Collection("", output->collection_names[i], output->type_name, output->level));
            }
        }
    }

};


} // namespace jana::components
