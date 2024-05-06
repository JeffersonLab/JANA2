// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Omni/JCollection.h>
#include <podio/podioVersion.h>
#include <podio/Frame.h>


class JPodioCollection : public JCollection {   

    // Fields
    const podio::CollectionBase* m_collection = nullptr;
    podio::Frame* m_frame = nullptr;

    // Getters
    template <typename T>
    const T* GetCollection();

    // Setters
    void SetFrame(podio::Frame* frame) { m_frame = frame; }

    template <typename CollectionT>
    void SetCollection(std::unique_ptr<CollectionT> collection);

    template <typename CollectionT>
    void SetCollectionAlreadyInFrame(const CollectionT* collection);
};


template <typename CollectionT>
void JPodioCollection::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (this->m_frame == nullptr) {
        throw JException("JPodioCollection: Unable to add collection to frame as frame is missing!");
    }
    this->m_frame->put(std::move(collection), this->GetCollectionName());
    const auto* moved = &this->m_frame->template get<CollectionT>(this->GetCollectionName());
    this->m_collection = moved;

    this->SetCreationStatus(JCollection::CreationStatus::Inserted);
}

template <typename CollectionT>
void JPodioCollection::SetCollectionAlreadyInFrame(const CollectionT* collection) {
    m_collection = collection;
    SetCreationStatus(JPodioCollection::CreationStatus::Inserted);
}


