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
    JPodioDatabundle* m_podio_databundle;
public:
    PodioOutput(JHasDatabundleOutputs* owner, std::string unique_name="") {

        owner->RegisterOutput(this);
        this->m_podio_databundle = new JPodioDatabundle;
        this->databundles.push_back(m_podio_databundle);

        this->type_name = JTypeInfo::demangle<PodioT>();
        m_podio_databundle->SetTypeName(JTypeInfo::demangle<PodioT>());

        m_podio_databundle->SetUniqueName(unique_name);
        this->databundle_names.push_back(unique_name);

        m_transient_collection = std::move(std::make_unique<typename PodioT::collection_type>());
    }

    std::unique_ptr<typename PodioT::collection_type>& operator()() { return m_transient_collection; }

    JPodioDatabundle* GetDatabundle() const { return m_podio_databundle; }

    void SetUniqueName(std::string unique_name) {
        this->m_podio_databundle->SetUniqueName(unique_name);
        this->databundle_names.clear();
        this->databundle_names.push_back(unique_name);
    }

    void SetShortName(std::string short_name) {
        this->m_podio_databundle->SetShortName(short_name);
        this->databundle_names.clear();
        this->databundle_names.push_back(this->m_podio_databundle->GetUniqueName());
    }

protected:

    void StoreData(JFactorySet& facset) override {

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
        for (auto& coll_name : this->databundle_names) {
            m_transient_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
    void StoreData(JFactorySet& facset) override {
        if (m_transient_collections.size() != this->databundle_names.size()) {
            throw JException("VariadicPodioOutput InsertCollection failed: Declared %d collections, but provided %d.", this->databundle_names.size(), m_transient_collections.size());
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
            i += 1;
        }
        m_transient_collections.clear();
    }

    void Reset() override {
        for (auto& coll_name : this->databundle_names) {
            m_transient_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }
};


} // namespace jana::components

