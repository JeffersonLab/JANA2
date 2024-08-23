// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Utils/JTypeInfo.h"
#include <JANA/Components/JCollection.h>
#include <podio/CollectionBase.h>
#include <podio/podioVersion.h>
#include <podio/Frame.h>


class JPodioCollection : public JCollection {   

private:
    // Fields
    const podio::CollectionBase* m_collection = nullptr;
    podio::Frame* m_frame = nullptr;

public:
    // Getters
    const podio::CollectionBase* GetCollection() const { return m_collection; }

    template <typename T> const typename T::collection_type* GetCollection();


    // Setters
    void SetFrame(podio::Frame* frame) { m_frame = frame; }

    template <typename T>
    void SetCollection(std::unique_ptr<typename T::collection_type> collection);

    template <typename T>
    void SetCollectionAlreadyInFrame(const typename T::collection_type* collection);
};

template <typename T>
const typename T::collection_type* JPodioCollection::GetCollection() {
    assert(JTypeInfo::demangle<T>() == this->GetTypeName());
    return dynamic_cast<const typename T::collection_type*>(m_collection);
}


template <typename T>
void JPodioCollection::SetCollection(std::unique_ptr<typename T::collection_type> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (this->m_frame == nullptr) {
        throw JException("JPodioCollection: Unable to add collection to frame as frame is missing!");
    }
    this->m_frame->put(std::move(collection), this->GetCollectionName());
    const auto* moved = &this->m_frame->template get<typename T::collection_type>(this->GetCollectionName());
    this->m_collection = moved;

    this->SetTypeName(JTypeInfo::demangle<T>());
    this->SetCreationStatus(JCollection::CreationStatus::Inserted);
}

template <typename T>
void JPodioCollection::SetCollectionAlreadyInFrame(const typename T::collection_type* collection) {
    m_collection = collection;
    this->SetTypeName(JTypeInfo::demangle<T>());
    SetCreationStatus(JPodioCollection::CreationStatus::Inserted);
}


