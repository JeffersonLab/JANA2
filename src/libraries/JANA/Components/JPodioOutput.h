#pragma once
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JFactorySet.h>
#include <JANA/Components/JHasOutputs.h>
#include <JANA/Components/JPodioDatabundle.h>
#include <JANA/Components/JLightweightDatabundle.h>
#include <podio/Frame.h>
#include <memory>


namespace jana::components {


template <typename PodioT>
class PodioOutput : public JHasOutputs::OutputBase {
private:
    std::unique_ptr<typename PodioT::collection_type> m_transient_collection;
    bool m_is_subset = false;
    JPodioDatabundle* m_podio_databundle;

public:
    PodioOutput(JHasOutputs* owner, std::string collection_name="") {

        owner->RegisterOutput(this);
        this->m_podio_databundle = new JPodioDatabundle;
        SetDatabundle(m_podio_databundle);

        m_podio_databundle->SetShortName(collection_name);
        m_podio_databundle->SetTypeName(JTypeInfo::demangle<PodioT>());
        m_podio_databundle->SetTypeIndex(std::type_index(typeid(PodioT)));

        m_transient_collection = std::move(std::make_unique<typename PodioT::collection_type>());
    }

    std::unique_ptr<typename PodioT::collection_type>& operator()() { return m_transient_collection; }

    typename PodioT::collection_type* operator->() { return m_transient_collection.get(); }

    JPodioDatabundle* GetDatabundle() const { return m_podio_databundle; }

    void SetSubsetCollection(bool is_subset) { 
        m_is_subset = is_subset; 
        m_transient_collection->setSubsetCollection(is_subset);
    }

    bool IsSubsetCollection() const { return m_is_subset; }


protected:
    void ClearData() {
        m_podio_databundle->ClearData();
    }

    void LagrangianStore(JFactorySet& facset, JDatabundle::Status status) override {

        auto* bundle = facset.GetDatabundle("podio::Frame");
        JLightweightDatabundleT<podio::Frame>* typed_bundle = nullptr;

        if (bundle == nullptr) {
            // LOG << "No frame databundle found. Creating new databundle.";
            typed_bundle = new JLightweightDatabundleT<podio::Frame>;
            facset.Add(typed_bundle);
        }
        else {
            typed_bundle = dynamic_cast<JLightweightDatabundleT<podio::Frame>*>(bundle);
            if (typed_bundle == nullptr) {
                throw JException("Databundle with unique_name 'podio::Frame' is not a JLightweightDatabundleT");
            }
        }
        if (typed_bundle->GetSize() == 0) {
            // LOG << "Found typed bundle with no frame. Creating new frame.";
            typed_bundle->GetData().push_back(new podio::Frame);
            typed_bundle->SetStatus(JDatabundle::Status::Inserted);
        }
        podio::Frame* frame = typed_bundle->GetData().at(0);

        // LOG << "Storing podio collection with name=" << m_podio_databundle->GetUniqueName() << " to frame " << frame << "...";
        frame->put(std::move(m_transient_collection), m_podio_databundle->GetUniqueName());
        // LOG << "...done";
        const auto* moved = &frame->template get<typename PodioT::collection_type>(m_podio_databundle->GetUniqueName());
        m_podio_databundle->SetCollection(moved);
        m_podio_databundle->SetStatus(status);
        m_transient_collection = std::make_unique<typename PodioT::collection_type>();
    }


    void EulerianStore(JFactorySet& facset) override {

        // First we retrieve the podio::Frame

        auto* frame_bundle = facset.GetDatabundle("podio::Frame");
        JLightweightDatabundleT<podio::Frame>* typed_frame_bundle = nullptr;

        if (frame_bundle == nullptr) {
            // LOG << "No frame databundle found. Creating new databundle.";
            typed_frame_bundle = new JLightweightDatabundleT<podio::Frame>;
            facset.Add(typed_frame_bundle);
        }
        else {
            typed_frame_bundle = dynamic_cast<JLightweightDatabundleT<podio::Frame>*>(frame_bundle);
            if (typed_frame_bundle == nullptr) {
                throw JException("Databundle with unique_name 'podio::Frame' is not a JLightweightDatabundleT");
            }
        }
        if (typed_frame_bundle->GetSize() == 0) {
            // LOG << "Found typed bundle with no frame. Creating new frame.";
            typed_frame_bundle->GetData().push_back(new podio::Frame);
            typed_frame_bundle->SetStatus(JDatabundle::Status::Inserted);
        }
        podio::Frame* frame = typed_frame_bundle->GetData().at(0);
        frame->put(std::move(m_transient_collection), m_podio_databundle->GetUniqueName());
        const auto* published = &frame->template get<typename PodioT::collection_type>(m_podio_databundle->GetUniqueName());

        // Then we store the collection itself

        JPodioDatabundle* typed_collection_bundle = nullptr;
        auto collection_bundle = facset.GetDatabundle(m_podio_databundle->GetTypeIndex(), m_podio_databundle->GetUniqueName());

        if (collection_bundle == nullptr) {
            // No databundle present. In this case we simply use m_podio_databundle as a template
            typed_collection_bundle = new JPodioDatabundle(*m_podio_databundle);
            facset.Add(typed_collection_bundle);
        }
        else {
            typed_collection_bundle = dynamic_cast<JPodioDatabundle*>(collection_bundle);
            if (typed_collection_bundle == nullptr) {
                // Wrong databundle present
                throw JException("Databundle with unique_name '%s' is not a JPodioDatabundle", m_podio_databundle->GetUniqueName().c_str());
            }
        }
        typed_collection_bundle->SetCollection(published);
        typed_collection_bundle->SetStatus(JDatabundle::Status::Inserted);
        m_transient_collection = std::make_unique<typename PodioT::collection_type>();
        m_transient_collection->setSubsetCollection(m_is_subset);
    }
};


template <typename PodioT>
class VariadicPodioOutput : public JHasOutputs::VariadicOutputBase {
private:
    std::vector<std::unique_ptr<typename PodioT::collection_type>> m_transient_collections;
    std::vector<JPodioDatabundle*> m_databundles;

public:
    VariadicPodioOutput(JHasOutputs* owner, std::vector<std::string> default_collection_names={}) {
        owner->RegisterOutput(this);
        for (const std::string& name : default_collection_names) {
            auto databundle = new JPodioDatabundle;
            databundle->SetUniqueName(name);
            databundle->SetTypeName(JTypeInfo::demangle<PodioT>());
            databundle->SetTypeIndex(std::type_index(typeid(PodioT)));
            GetDatabundles().push_back(databundle);
            m_databundles.push_back(databundle);
            m_transient_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }

    void SetShortNames(std::vector<std::string> short_names) override {
        m_databundles.clear();
        m_transient_collections.clear();
        for (const std::string& name : short_names) {
            auto databundle = new JPodioDatabundle;
            databundle->SetShortName(name);
            databundle->SetTypeName(JTypeInfo::demangle<PodioT>());
            databundle->SetTypeIndex(std::type_index(typeid(PodioT)));
            GetDatabundles().push_back(databundle);
            m_databundles.push_back(databundle);
            m_transient_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }

    void SetUniqueNames(std::vector<std::string> unique_names) override {
        m_databundles.clear();
        m_transient_collections.clear();
        for (const std::string& name : unique_names) {
            auto databundle = new JPodioDatabundle;
            databundle->SetUniqueName(name);
            databundle->SetTypeName(JTypeInfo::demangle<PodioT>());
            databundle->SetTypeIndex(std::type_index(typeid(PodioT)));
            GetDatabundles().push_back(databundle);
            m_databundles.push_back(databundle);
            m_transient_collections.push_back(std::make_unique<typename PodioT::collection_type>());
        }
    }

    std::vector<std::unique_ptr<typename PodioT::collection_type>>& operator()() { return m_transient_collections; }

    void LagrangianStore(JFactorySet& facset, JDatabundle::Status status) override {
        if (m_transient_collections.size() != GetDatabundles().size()) {
            throw JException("VariadicPodioOutput::LagrangianStore() failed: Declared %d collections, but provided %d.", GetDatabundles().size(), m_transient_collections.size());
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
            const auto &databundle = dynamic_cast<JPodioDatabundle*>(m_databundles[i]);
            databundle->SetCollection(moved);
            databundle->SetStatus(status);
            i += 1;
            collection = std::make_unique<typename PodioT::collection_type>();
        }
    }

    void EulerianStore(JFactorySet& facset) override {

        // First we retrieve the podio::Frame

        auto* frame_bundle = facset.GetDatabundle("podio::Frame");
        JLightweightDatabundleT<podio::Frame>* typed_frame_bundle = nullptr;

        if (frame_bundle == nullptr) {
            // LOG << "No frame databundle found. Creating new databundle.";
            typed_frame_bundle = new JLightweightDatabundleT<podio::Frame>;
            facset.Add(typed_frame_bundle);
        }
        else {
            typed_frame_bundle = dynamic_cast<JLightweightDatabundleT<podio::Frame>*>(frame_bundle);
            if (typed_frame_bundle == nullptr) {
                throw JException("Databundle with unique_name 'podio::Frame' is not a JLightweightDatabundleT");
            }
        }
        if (typed_frame_bundle->GetSize() == 0) {
            // LOG << "Found typed bundle with no frame. Creating new frame.";
            typed_frame_bundle->GetData().push_back(new podio::Frame);
            typed_frame_bundle->SetStatus(JDatabundle::Status::Inserted);
        }
        podio::Frame* frame = typed_frame_bundle->GetData().at(0);


        int i=0;
        for (auto& collection : m_transient_collections) {
            frame->put(std::move(collection), m_databundles[i]->GetUniqueName());
            const auto* moved = &frame->template get<typename PodioT::collection_type>(m_databundles[i]->GetUniqueName());

            JPodioDatabundle* typed_collection_bundle = nullptr;
            auto collection_bundle = facset.GetDatabundle(m_databundles[i]->GetTypeIndex(), m_databundles[i]->GetUniqueName());

            if (collection_bundle == nullptr) {
                // No databundle present. In this case we create it, using m_databundles[i] as a template
                typed_collection_bundle = new JPodioDatabundle(*m_databundles[i]);
                facset.Add(typed_collection_bundle);
            }
            else {
                typed_collection_bundle = dynamic_cast<JPodioDatabundle*>(collection_bundle);
                if (typed_collection_bundle == nullptr) {
                    // Wrong databundle present
                    throw JException("Databundle with unique_name '%s' is not a JPodioDatabundle", m_databundles[i]->GetUniqueName().c_str());
                }
            }

            // Then we store the collection itself
            typed_collection_bundle->SetCollection(moved);
            typed_collection_bundle->SetStatus(JDatabundle::Status::Inserted);
            i += 1;
        }
        m_transient_collections.clear();
    }
};

} // namespace jana::components

template <typename T> using PodioOutput = jana::components::PodioOutput<T>;
template <typename T> using VariadicPodioOutput = jana::components::VariadicPodioOutput<T>;

