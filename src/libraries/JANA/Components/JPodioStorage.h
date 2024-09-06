// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Utils/JTypeInfo.h"
#include <JANA/Components/JStorage.h>
#include <podio/CollectionBase.h>
#include <podio/podioVersion.h>
#include <podio/Frame.h>


class JPodioStorage : public JStorage {   

private:
    const podio::CollectionBase* m_collection = nullptr;
    podio::Frame* m_frame = nullptr;

public:
    size_t GetSize() const override {
        return m_collection->size();
    }

    virtual void ClearData() override {
        m_collection = nullptr;
        m_frame = nullptr;
        // Podio clears the data itself when the frame is destroyed.
        // Until then, the collection is immutable.
        //
        // Consider: Instead of putting the frame in its own JFactory, maybe we 
        // want to maintain a shared_ptr to the frame here, and delete the
        // the reference on ClearData(). Thus, the final call to ClearData()
        // for each events deletes the frame and actually frees the data. 
        // This would let us support multiple frames within one event, though 
        // it might also prevent the user from accessing frames directly.
    }

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
const typename T::collection_type* JPodioStorage::GetCollection() {
    assert(JTypeInfo::demangle<T>() == this->GetTypeName());
    return dynamic_cast<const typename T::collection_type*>(m_collection);
}


template <typename T>
void JPodioStorage::SetCollection(std::unique_ptr<typename T::collection_type> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (this->m_frame == nullptr) {
        throw JException("JPodioStorage: Unable to add collection to frame as frame is missing!");
    }
    this->m_frame->put(std::move(collection), this->GetCollectionName());
    const auto* moved = &this->m_frame->template get<typename T::collection_type>(this->GetCollectionName());
    this->m_collection = moved;

    this->SetTypeName(JTypeInfo::demangle<T>());
    this->SetCreationStatus(JStorage::CreationStatus::Inserted);
}

template <typename T>
void JPodioStorage::SetCollectionAlreadyInFrame(const typename T::collection_type* collection) {
    m_collection = collection;
    this->SetTypeName(JTypeInfo::demangle<T>());
    SetCreationStatus(JPodioStorage::CreationStatus::Inserted);
}


