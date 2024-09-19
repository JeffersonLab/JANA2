#pragma once
#include <JANA/Components/JPodioStorage.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JEvent.h>
#include <JANA/Components/JHasFactoryOutputs.h>
#include <memory>


namespace jana::components {


template <typename PodioT>
class PodioOutput : public JHasFactoryOutputs::OutputBase {
private:
    std::unique_ptr<typename PodioT::collection_type> m_data;
    JPodioStorage* m_collection;
public:
    PodioOutput(JHasFactoryOutputs* owner, std::string default_collection_name="") {
        owner->RegisterOutput(this);
        auto coll = std::make_unique<JPodioStorage>();
        coll->SetCollectionName(default_collection_name);
        coll->SetTypeName(JTypeInfo::demangle<PodioT>());
        m_collection = coll.get();
        m_collections.push_back(std::move(coll));
        m_data = std::move(std::make_unique<typename PodioT::collection_type>());
    }

    std::unique_ptr<typename PodioT::collection_type>& operator()() { return m_data; }

    const JStorage* GetCollection() const { return m_collection; }


protected:
    //void CreateCollections() override {
    //}

    void PutCollections(const JEvent& event) override {
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

        frame->put(std::move(m_data), m_collection->GetCollectionName());
        const auto* moved = &frame->template get<typename PodioT::collection_type>(m_collection->GetCollectionName());
        m_data = nullptr;
        m_collection->SetCollectionAlreadyInFrame<PodioT>(moved);
        //m_collection->SetFrame(frame); // We might not need this!
    }
    void Reset() override {
        m_data = std::move(std::make_unique<typename PodioT::collection_type>());
    }
};


template <typename PodioT>
class VariadicPodioOutput : public JHasFactoryOutputs::OutputBase {
private:
    std::vector<std::unique_ptr<typename PodioT::collection_type>> m_data;

public:
    VariadicPodioOutput(JHasFactoryOutputs* owner, std::vector<std::string> default_collection_names={}) {
        owner->RegisterOutput(this);
        this->m_is_variadic = true;
        for (const std::string& name : default_collection_names) {
            auto coll = std::make_unique<JPodioStorage>();
            coll->SetCollectionName(name);
            coll->SetTypeName(JTypeInfo::demangle<PodioT>());
            m_collections.push_back(std::move(coll));
        }
        for (auto& coll_name : this->collection_names) {
            m_data.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
    void PutCollections(const JEvent& event) override {
        if (m_data.size() != this->collection_names.size()) {
            throw JException("VariadicPodioOutput InsertCollection failed: Declared %d collections, but provided %d.", this->collection_names.size(), m_data.size());
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
        for (auto& datum : m_data) {
            frame->put(std::move(std::move(datum)), m_collections[i]->GetCollectionName());
            const auto* moved = &frame->template get<typename PodioT::collection_type>(m_collections[i]->GetCollectionName());
            datum = nullptr;
            const auto &coll = dynamic_cast<JPodioStorage>(m_collections[i]);
            coll.SetCollectionAlreadyInFrame<PodioT>(moved);
        }
    }
    void Reset() override {
        m_data.clear();
        for (auto& coll : this->m_collections) {
            coll->ClearData();
        }
        for (auto& coll_name : this->collection_names) {
            m_data.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
};


} // namespace jana::components

