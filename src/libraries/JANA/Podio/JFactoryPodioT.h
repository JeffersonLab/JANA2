
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include <JANA/JFactory.h>
#include <JANA/Omni/JPodioCollection.h>
#include <podio/podioVersion.h>
#include <podio/Frame.h>

#if podio_VERSION < PODIO_VERSION(0, 17, 0)
template <typename S> struct PodioTypeMap;
#endif


/// The point of this additional base class is to allow us _untyped_ access to the underlying PODIO collection,
/// at the cost of some weird multiple inheritance. The JEvent can trigger the untyped factory using Create(), then
///
class JFactoryPodio : public JFactory {
protected:
    bool mIsSubsetCollection = false;
    JPodioCollection* mCollection;

public:
    // Meant to be called internally, from JMultifactory
    friend class JMultifactory;
    void SetFrame(podio::Frame* frame) { mCollection->SetFrame(frame); }

    // Meant to be called internally, from JEvent:
    friend class JEvent;
    const podio::CollectionBase* GetCollection() { return mCollection->GetCollection(); }

    // Meant to be called from ctor, or externally, if we are creating a dummy factory such as a multifactory helper
    void SetSubsetCollection(bool isSubsetCollection=true) { mIsSubsetCollection = isSubsetCollection; }
};



template <typename T>
class JFactoryPodioT : public JFactoryPodio {
public:
#if podio_VERSION >= PODIO_VERSION(0, 17, 0)
    using CollectionT = typename T::collection_type;
#else
    using CollectionT = typename PodioTypeMap<T>::collection_t;
#endif
private:
    // mCollection is owned by the frame.
    // mFrame is owned by the JFactoryT<podio::Frame>.
    // mData holds lightweight value objects which hold a pointer into mCollection.
    // This factory owns these value objects.

public:
    explicit JFactoryPodioT();
    ~JFactoryPodioT() override;

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void ChangeRun(const std::shared_ptr<const JEvent>&) override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Finish() override {}

    void Create(const std::shared_ptr<const JEvent>& event) final;
    std::type_index GetObjectType() const final { return std::type_index(typeid(T)); }
    std::size_t GetNumObjects() const final { return mCollection->GetSize(); }
    void ClearData() final;

    void SetCollection(CollectionT&& collection);
    void SetCollection(std::unique_ptr<CollectionT> collection);


private:
    // This is meant to be called by JEvent::Insert
    friend class JEvent;
    void SetCollectionAlreadyInFrame(const CollectionT* collection);
};


template <typename T>
JFactoryPodioT<T>::JFactoryPodioT() {
    SetObjectName(JTypeInfo::demangle<T>()); 
    mCollection = new JPodioCollection();
    mCollections.push_back(mCollection);
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

    if (mCollection->GetFrame() == nullptr) {
        throw JException("JFactoryPodioT: Unable to add collection to frame as frame is missing!");
    }
    mCollection->SetCollection<T>(std::move(collection));
    mCollection->SetCreationStatus(JCollection::CreationStatus::Inserted);

    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}


template <typename T>
void JFactoryPodioT<T>::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (mCollection->GetFrame() == nullptr) {
        throw JException("JFactoryPodioT: Unable to add collection to frame as frame is missing!");
    }
    mCollection->SetCollection<T>(std::move(collection));
    mCollection->SetCreationStatus(JCollection::CreationStatus::Inserted);

    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}


template <typename T>
void JFactoryPodioT<T>::ClearData() {
    if (this->mStatus == JFactory::Status::Uninitialized) {
        return;
    }
    this->mStatus = JFactory::Status::Unprocessed;
    this->mCreationStatus = JFactory::CreationStatus::NotCreatedYet;
}

template <typename T>
void JFactoryPodioT<T>::SetCollectionAlreadyInFrame(const CollectionT* collection) {
    this->mCollection->SetCollectionAlreadyInFrame<T>(collection);
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}

// This free function is used to break the dependency loop between JFactoryPodioT and JEvent.
podio::Frame* GetOrCreateFrame(const std::shared_ptr<const JEvent>& event);

template <typename T>
void JFactoryPodioT<T>::Create(const std::shared_ptr<const JEvent>& event) {
    SetFrame(GetOrCreateFrame(event));
    try {
        JFactory::Create(event);
    }
    catch (...) {
        if (mCollection == nullptr) {
            // If calling Create() excepts, we still create an empty collection
            // so that podio::ROOTFrameWriter doesn't segfault on the null mCollection pointer
            SetCollection(CollectionT());
        }
        throw;
    }
    if (mCollection == nullptr) {
        SetCollection(CollectionT());
        // If calling Process() didn't result in a call to Set() or SetCollection(), we create an empty collection
        // so that podio::ROOTFrameWriter doesn't segfault on the null mCollection pointer
    }
}

