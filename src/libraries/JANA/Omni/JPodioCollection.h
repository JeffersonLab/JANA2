// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Omni/JCollection.h>
#include <podio/podioVersion.h>
#include <podio/Fame.h>


class JPodioCollection : public JCollection {   

    // Fields
    const podio::CollectionBase* m_collection = nullptr;
    podio::Frame* m_frame = nullptr;

    // Getters
    const podio::CollectionBase* GetCollection() { return m_collection; }

    // Setters
    void SetFrame(podio::Frame* frame) { m_frame = frame; }

    template <typename T>
    void SetCollection(std::unique_ptr<CollectionT> collection);

    template <typename T>
    void SetCollectionAlreadyInFrame(const CollectionT* collection);
}



template <typename T>
void JFactoryPodioT<T>::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (this->mFrame == nullptr) {
        throw JException("JPodioCollection: Unable to add collection to frame as frame is missing!");
    }
    this->mFrame->put(std::move(collection), this->GetTag());
    const auto* moved = &this->mFrame->template get<CollectionT>(this->GetTag());
    this->mCollection = moved;

    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}

template <typename T>
void JFactoryPodioT<T>::SetCollectionAlreadyInFrame(const CollectionT* collection) {
    m_collection = collection;
    SetStatus(J)actory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}


