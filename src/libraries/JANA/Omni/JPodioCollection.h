// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Omni/JCollection.h>
#include <podio/podioVersion.h>
#include <podio/Frame.h>

#include <unordered_map>


class JPodioCollection : public JCollection {   

    // Fields
    const podio::CollectionBase* m_collection = nullptr;
    podio::Frame* m_frame = nullptr;
    bool m_owns_data = true;

public:
    // Getters
    const podio::CollectionBase* GetCollection() { return m_collection; }
    
    podio::Frame* GetFrame() { return m_frame; }

    // Setters
    void SetFrame(podio::Frame* frame) { m_frame = frame; }

    template <typename T>
    void SetCollection(std::unique_ptr<typename T::collection_type> collection);

    template <typename T>
    void SetCollection(typename T::collection_type&& collection);

    template <typename T>
    void SetCollectionAlreadyInFrame(const typename T::collection_type* collection);
    

    size_t GetSize() const override { return m_collection->size(); }

    void ClearData() override { 
        m_collection = nullptr; 
        m_frame = nullptr; 
    };
};



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
    SetCreationStatus(CreationStatus::Inserted);
}

template <typename T>
void JPodioCollection::SetCollection(typename T::collection_type&& collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (this->m_frame == nullptr) {
        throw JException("JPodioCollection: Unable to add collection to frame as frame is missing!");
    }
    this->m_frame->put(std::move(collection), this->GetCollectionName());
    const auto* moved = &this->m_frame->template get<typename T::collection_type>(this->GetCollectionName());
    this->m_collection = moved;
    SetCreationStatus(CreationStatus::Inserted);
}

template <typename T>
void JPodioCollection::SetCollectionAlreadyInFrame(const typename T::collection_type* collection) {
    m_collection = collection;
    SetCreationStatus(CreationStatus::Inserted);
}


