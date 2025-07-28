#pragma once
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JFactorySet.h>
#include <JANA/Components/JHasDatabundleOutputs.h>
#include <JANA/Components/JPodioDatabundle.h>
#include <JANA/Components/JLightweightDatabundle.h>
#include <podio/Frame.h>
#include <memory>


namespace jana::components {


template <typename PodioT>
class PodioOutput : public JHasDatabundleOutputs::OutputBase {
private:
    std::unique_ptr<typename PodioT::collection_type> m_transient_collection;
    bool m_is_subset = false;
    JPodioDatabundle* m_podio_databundle;

public:
    PodioOutput(JHasDatabundleOutputs* owner) {

        owner->RegisterOutput(this);
        this->m_podio_databundle = new JPodioDatabundle;
        this->GetDatabundles().push_back(m_podio_databundle);

        m_podio_databundle->SetShortName("");
        m_podio_databundle->SetTypeName(JTypeInfo::demangle<PodioT>());
        m_podio_databundle->SetTypeIndex(std::type_index(typeid(PodioT)));

        m_transient_collection = std::move(std::make_unique<typename PodioT::collection_type>());
    }

    std::unique_ptr<typename PodioT::collection_type>& operator()() { return m_transient_collection; }

    typename PodioT::collection_type* operator->() { return m_transient_collection.get(); }

    JPodioDatabundle* GetDatabundle() const { return m_podio_databundle; }

    void SetSubsetCollection(bool is_subset) { m_is_subset = is_subset; }
    bool IsSubsetCollection() const { return m_is_subset; }

    void SetUniqueName(std::string unique_name) {
        this->m_podio_databundle->SetUniqueName(unique_name);
    }

    void SetShortName(std::string short_name) {
        this->m_podio_databundle->SetShortName(short_name);
    }

protected:

    void StoreData(JFactorySet& facset) override {

        auto* bundle = facset.GetDatabundle("podio::Frame");
        JLightweightDatabundleT<podio::Frame>* typed_bundle = nullptr;

        if (bundle == nullptr) {
            LOG << "No frame databundle found. Creating new databundle.";
            typed_bundle = new JLightweightDatabundleT<podio::Frame>;
            typed_bundle->UseSelfContainedData();
            facset.Add(typed_bundle);
        }
        else {
            typed_bundle = dynamic_cast<JLightweightDatabundleT<podio::Frame>*>(bundle);
            if (typed_bundle == nullptr) {
                throw JException("Databundle with unique_name 'podio::Frame' is not a JLightweightDatabundleT");
            }
        }
        if (typed_bundle->GetSize() == 0) {
            LOG << "Found typed bundle with no frame. Creating new frame.";
            typed_bundle->GetData().push_back(new podio::Frame);
            typed_bundle->SetStatus(JDatabundle::Status::Inserted);
        }
        podio::Frame* frame = typed_bundle->GetData().at(0);

        LOG << "Storing podio collection with name=" << m_podio_databundle->GetUniqueName() << " to frame " << frame << "...";
        frame->put(std::move(m_transient_collection), m_podio_databundle->GetUniqueName());
        LOG << "...done";
        const auto* moved = &frame->template get<typename PodioT::collection_type>(m_podio_databundle->GetUniqueName());
        m_podio_databundle->SetCollection(moved);
        m_podio_databundle->SetStatus(JDatabundle::Status::Created);
        m_transient_collection = std::make_unique<typename PodioT::collection_type>();
    }

    void Reset() override {
        m_transient_collection = std::move(std::make_unique<typename PodioT::collection_type>());
        m_transient_collection->setSubsetCollection(m_is_subset);
    }
};


template <typename PodioT>
class VariadicPodioOutput : public JHasDatabundleOutputs::OutputBase {
private:
    std::vector<std::unique_ptr<typename PodioT::collection_type>> m_transient_collections;
    std::vector<JPodioDatabundle*> m_databundles;

public:
    VariadicPodioOutput(JHasDatabundleOutputs* owner, std::vector<std::string> default_collection_names={}) {
        owner->RegisterOutput(this);
        this->is_variadic = true;
        for (const std::string& name : default_collection_names) {
            auto coll = std::make_unique<JPodioDatabundle>();
            coll->SetUniqueName(name);
            coll->SetTypeName(JTypeInfo::demangle<PodioT>());
            m_transient_collections.push_back(std::move(coll));
        }
        for (auto& coll_name : GetDatabundles()) {
            m_transient_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
    void StoreData(JFactorySet& facset) override {
        if (m_transient_collections.size() != GetDatabundles().size()) {
            throw JException("VariadicPodioOutput InsertCollection failed: Declared %d collections, but provided %d.", GetDatabundles().size(), m_transient_collections.size());
        }

        auto* bundle = facset.GetDatabundle("podio::Frame");
        JLightweightDatabundleT<podio::Frame>* typed_bundle = nullptr;

        if (bundle == nullptr) {
            typed_bundle = new JLightweightDatabundleT<podio::Frame>;
            facset.Add(typed_bundle);
        }
        else {
            typed_bundle = dynamic_cast<JLightweightDatabundleT<podio::Frame>*>(bundle);
        }
        if (typed_bundle->GetSize() == 0) {
            typed_bundle->GetData().push_back(new podio::Frame);
            typed_bundle->SetStatus(JDatabundle::Status::Inserted);
        }
        podio::Frame* frame = typed_bundle->GetData().at(0);

        size_t i = 0;
        for (auto& collection : m_transient_collections) {
            frame->put(std::move(collection), m_databundles[i]->GetUniqueName());
            const auto* moved = &frame->template get<typename PodioT::collection_type>(m_databundles[i]->GetUniqueName());
            collection = nullptr;
            const auto &databundle = dynamic_cast<JPodioDatabundle*>(m_databundles[i]);
            databundle->SetCollection(moved);
            databundle->SetStatus(JDatabundle::Status::Created);
            i += 1;
        }
        m_transient_collections.clear();
    }

    void Reset() override {
        for (auto& coll_name : GetDatabundles()) {
            m_transient_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
};


} // namespace jana::components

