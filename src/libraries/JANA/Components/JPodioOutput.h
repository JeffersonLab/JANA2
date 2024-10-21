#pragma once
#include <JANA/Components/JPodioDataBundle.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JEvent.h>
#include <JANA/Components/JHasFactoryOutputs.h>
#include <podio/Frame.h>
#include <memory>


namespace jana::components {


template <typename PodioT>
class PodioOutput : public JHasFactoryOutputs::OutputBase {
private:
    std::unique_ptr<typename PodioT::collection_type> m_transient_collection;
    JPodioDataBundle* m_podio_databundle;
public:
    PodioOutput(JHasFactoryOutputs* owner, std::string default_collection_name="") {
        owner->RegisterOutput(this);
        auto bundle = std::make_unique<JPodioDataBundle>();
        bundle->SetUniqueName(default_collection_name);
        bundle->SetTypeName(JTypeInfo::demangle<PodioT>());
        m_podio_databundle = bundle.get();
        m_databundles.push_back(std::move(bundle));
        m_transient_collection = std::move(std::make_unique<typename PodioT::collection_type>());
    }

    std::unique_ptr<typename PodioT::collection_type>& operator()() { return m_transient_collection; }

    JPodioDataBundle* GetDataBundle() const { return m_podio_databundle; }


protected:

    void StoreData(const JEvent& event) override {
        podio::Frame* frame;
        try {
            frame = const_cast<podio::Frame*>(event.GetSingle<podio::Frame>());
            if (frame == nullptr) {
                frame = new podio::Frame;
                event.Insert<podio::Frame>(frame);
            }
        }
        catch (...) {
            frame = new podio::Frame;
            event.Insert<podio::Frame>(frame);
        }

        frame->put(std::move(m_transient_collection), m_podio_databundle->GetUniqueName());
        const auto* moved = &frame->template get<typename PodioT::collection_type>(m_podio_databundle->GetUniqueName());
        m_transient_collection = nullptr;
        m_podio_databundle->SetCollection(moved);
    }
    void Reset() override {
        m_transient_collection = std::move(std::make_unique<typename PodioT::collection_type>());
    }
};


template <typename PodioT>
class VariadicPodioOutput : public JHasFactoryOutputs::OutputBase {
private:
    std::vector<std::unique_ptr<typename PodioT::collection_type>> m_collections;
    std::vector<JPodioDataBundle*> m_databundles;

public:
    VariadicPodioOutput(JHasFactoryOutputs* owner, std::vector<std::string> default_collection_names={}) {
        owner->RegisterOutput(this);
        this->m_is_variadic = true;
        for (const std::string& name : default_collection_names) {
            auto coll = std::make_unique<JPodioDataBundle>();
            coll->SetUniqueName(name);
            coll->SetTypeName(JTypeInfo::demangle<PodioT>());
            m_collections.push_back(std::move(coll));
        }
        for (auto& coll_name : this->collection_names) {
            m_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
    void StoreData(const JEvent& event) override {
        if (m_collections.size() != this->collection_names.size()) {
            throw JException("VariadicPodioOutput InsertCollection failed: Declared %d collections, but provided %d.", this->collection_names.size(), m_collections.size());
        }

        podio::Frame* frame;
        try {
            frame = const_cast<podio::Frame*>(event.GetSingle<podio::Frame>());
            if (frame == nullptr) {
                frame = new podio::Frame;
                event.Insert<podio::Frame>(frame);
            }
        }
        catch (...) {
            frame = new podio::Frame;
            event.Insert<podio::Frame>(frame);
        }

        size_t i = 0;
        for (auto& collection : m_collections) {
            frame->put(std::move(std::move(collection)), m_databundles[i]->GetUniqueName());
            const auto* moved = &frame->template get<typename PodioT::collection_type>(m_databundles[i]->GetUniqueName());
            collection = nullptr;
            const auto &databundle = dynamic_cast<JPodioDataBundle*>(m_databundles[i]);
            databundle->SetCollection(moved);
            i += 1;
        }
    }
    void Reset() override {
        m_collections.clear();
        for (auto& coll : this->m_databundles) {
            coll->ClearData();
        }
        for (auto& coll_name : this->collection_names) {
            m_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
};


} // namespace jana::components

