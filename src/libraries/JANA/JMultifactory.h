
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JFactory.h>
#include <JANA/Omni/JCollection.h>
#include <JANA/Omni/JBasicCollection.h>

#ifdef JANA2_HAVE_PODIO
#include <JANA/Podio/JPodioTypeHelpers.h>
#include <JANA/Omni/JPodioCollection.h>
#endif


class JMultifactory : public JFactory {


public:
    JMultifactory() = default;
    virtual ~JMultifactory() = default;

    // IMPLEMENTED BY USERS

    virtual void Init() {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    virtual void Process(const std::shared_ptr<const JEvent>&) {}
    void EndRun() override {}
    virtual void Finish() {}

    // CALLED BY USERS

    template <typename T>
    void DeclareOutput(std::string tag, bool owns_data=true);

    template <typename T>
    void SetData(std::string tag, std::vector<T*> data);

#ifdef JANA2_HAVE_PODIO

    template <typename T>
    void DeclarePodioOutput(std::string tag, bool owns_data=true);

    template <typename T>
    void SetCollection(std::string tag, std::unique_ptr<typename JFactoryPodioT<T>::CollectionT> collection);

#endif

};


template <typename T>
void JMultifactory::DeclareOutput(std::string tag, bool owns_data) {
    auto* coll = new JBasicCollectionT<T>();
    coll->SetCollectionName(std::move(tag));
    coll->SetTag(std::move(tag));
    coll->SetNotOwnerFlag(!owns_data);
    mCollections.push_back(coll);
}

template <typename T>
void JMultifactory::SetData(std::string tag, std::vector<T*> data) {
    bool found_collection = false;
    for (JCollection* coll : mCollections) {
        if (coll->GetCollectionName() != tag) continue;
        auto typed_coll = dynamic_cast<JBasicCollectionT<T>*>(coll);
        if (typed_coll == nullptr) {
            throw JException("Collection doesn't match type!");
        }
        typed_coll->GetData() = std::move(data);
    }
    if (!found_collection) {
        auto ex = JException("JMultifactory: Attempting to SetData() without corresponding DeclareOutput()");
        ex.function_name = "JMultifactory::SetData";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
}


#ifdef JANA2_HAVE_PODIO

template <typename T>
void JMultifactory::DeclarePodioOutput(std::string tag, bool owns_data) {

    auto* coll = new JPodioCollection<T>();
    coll->SetCollectionName(std::move(tag));
    coll->SetTag(std::move(tag));
    coll->SetOwnsDataFlag(owns_data);
    mCollections.push_back(coll);
    mNeedPodio = true;
}

template <typename T>
void JMultifactory::SetCollection(std::string tag, std::unique_ptr<typename JFactoryPodioT<T>::CollectionT> collection) {
    bool found_collection = false;
    for (JCollection* coll : mCollections) {
        if (coll->GetCollectionName() != tag) continue;
        auto typed_coll = dynamic_cast<JPodioCollection*>(coll);
        if (typed_coll == nullptr) {
            auto ex = JException("JMultifactory: Collection is not a JPodioCollection");
            ex.function_name = "JMultifactory::SetCollection";
            ex.type_name = m_type_name;
            ex.instance_name = m_prefix;
            ex.plugin_name = m_plugin_name;
            throw ex;
        }
        typed->SetFrame(mPodioFrame);
        typed->SetCollection(std::move(collection));
    }
    if (!found_collection) {
        auto ex = JException("JMultifactory: Attempting to SetCollection() without corresponding DeclarePodioOutput()");
        ex.function_name = "JMultifactory::SetCollection";
        ex.type_name = m_type_name;
        ex.instance_name = m_prefix;
        ex.plugin_name = m_plugin_name;
        throw ex;
    }
}

#endif // JANA2_HAVE_PODIO



