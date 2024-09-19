
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include "JANA/Utils/JTypeInfo.h"
#include <JANA/JFactory.h>
#include <JANA/Components/JPodioOutput.h>


template <typename T>
class JFactoryPodioT : public JFactory {
public:
    using CollectionT = typename T::collection_type;

private:
    jana::components::PodioOutput<T> m_output {this};

public:
    explicit JFactoryPodioT();
    ~JFactoryPodioT() override;

    void SetTag(std::string tag) { 
        mTag = tag;
        m_output.GetCollections().at(0)->SetCollectionName(tag);
    }

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void ChangeRun(const std::shared_ptr<const JEvent>&) override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Finish() override {}

    std::size_t GetNumObjects() const final { return m_output.GetCollection()->GetSize(); }
    void ClearData() final;

    void SetCollection(CollectionT&& collection);
    void SetCollection(std::unique_ptr<CollectionT> collection);
};


template <typename T>
JFactoryPodioT<T>::JFactoryPodioT() {
    mObjectName = JTypeInfo::demangle<T>();
}

template <typename T>
JFactoryPodioT<T>::~JFactoryPodioT() {
    // Ownership of mData, mCollection, and mFrame is complicated, so we always handle it via ClearData()
    ClearData();
}

template <typename T>
void JFactoryPodioT<T>::SetCollection(CollectionT&& collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    m_output() = std::make_unique<CollectionT>(std::move(collection));
}


template <typename T>
void JFactoryPodioT<T>::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    m_output() = std::move(collection);
}


template <typename T>
void JFactoryPodioT<T>::ClearData() {
    // Happens automatically now
}


