
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include <JANA/JFactoryT.h>
#include <podio/Frame.h>

/// The point of this additional base class is to allow us _untyped_ access to the underlying PODIO collection,
/// at the cost of some weird multiple inheritance. The JEvent can trigger the untyped factory using Create(), then
///
class JFactoryPodio {
protected:
    const podio::CollectionBase* mCollection = nullptr;
    bool mIsSubsetCollection = false;
    podio::Frame* mFrame = nullptr;

private:
    // Meant to be called internally, from JMultifactory
    friend class JMultifactory;
    void SetFrame(podio::Frame* frame) { mFrame = frame; }

    // Meant to be called internally, from JEvent:
    friend class JEvent;
    const podio::CollectionBase* GetCollection() { return mCollection; }

public:
    // Meant to be called from ctor, or externally, if we are creating a dummy factory such as a multifactory helper
    void SetSubsetCollection(bool isSubsetCollection=true) { mIsSubsetCollection = isSubsetCollection; }
};


template <typename T>
class JFactoryPodioT : public JFactoryT<T>, public JFactoryPodio {
public:
    using CollectionT = typename T::collection_type;
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
    std::size_t GetNumObjects() const final { return mCollection->size(); }
    void ClearData() final;

    void SetCollection(CollectionT&& collection);
    void SetCollection(std::unique_ptr<CollectionT> collection);
    void Set(const std::vector<T*>& aData) final;
    void Set(std::vector<T*>&& aData) final;
    void Insert(T* aDatum) final;



private:
    // This is meant to be called by JEvent::Insert
    friend class JEvent;
    void SetCollectionAlreadyInFrame(const CollectionT* collection);

};


template <typename T>
JFactoryPodioT<T>::JFactoryPodioT() = default;

template <typename T>
JFactoryPodioT<T>::~JFactoryPodioT() {
    // Ownership of mData, mCollection, and mFrame is complicated, so we always handle it via ClearData()
    ClearData();
}

template <typename T>
void JFactoryPodioT<T>::SetCollection(CollectionT&& collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (this->mFrame == nullptr) {
        throw JException("JFactoryPodioT: Unable to add collection to frame as frame is missing!");
    }
    const auto& moved = this->mFrame->put(std::move(collection), this->GetTag());
    this->mCollection = &moved;

    for (const T& item : moved) {
        T* clone = new T(item);
        this->mData.push_back(clone);
    }
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}


template <typename T>
void JFactoryPodioT<T>::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    if (this->mFrame == nullptr) {
        throw JException("JFactoryPodioT: Unable to add collection to frame as frame is missing!");
    }
    this->mFrame->put(std::move(collection), this->GetTag());
    const auto* moved = &this->mFrame->template get<CollectionT>(this->GetTag());
    this->mCollection = moved;

    for (const T& item : *moved) {
        T* clone = new T(item);
        this->mData.push_back(clone);
    }
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}


template <typename T>
void JFactoryPodioT<T>::ClearData() {
    if (this->mStatus == JFactory::Status::Uninitialized) {
        return;
    }
    for (auto p : this->mData) {
      // Avoid potentially invalid call to ObjBase::release(). The frame and
      // all the collections and all Obj may have been deallocated at this point.
      p->unlink();
      delete p;
    }
    this->mData.clear();
    this->mCollection = nullptr;  // Collection is owned by the Frame, so we ignore here
    this->mFrame = nullptr;  // Frame is owned by the JEvent, so we ignore here
    this->mStatus = JFactory::Status::Unprocessed;
    this->mCreationStatus = JFactory::CreationStatus::NotCreatedYet;
}

template <typename T>
void JFactoryPodioT<T>::SetCollectionAlreadyInFrame(const CollectionT* collection) {
    for (const T& item : *collection) {
        T* clone = new T(item);
        this->mData.push_back(clone);
    }
    this->mCollection = collection;
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}

// This free function is used to break the dependency loop between JFactoryPodioT and JEvent.
podio::Frame* GetOrCreateFrame(const std::shared_ptr<const JEvent>& event);

template <typename T>
void JFactoryPodioT<T>::Create(const std::shared_ptr<const JEvent>& event) {
    mFrame = GetOrCreateFrame(event);
    try {
        JFactory::Create(event);
    }
    catch (...) {
        if (mCollection == nullptr) {
            // If calling Create() excepts, we still create an empty collection
            // so that podio::ROOTWriter doesn't segfault on the null mCollection pointer
            SetCollection(CollectionT());
        }
        throw;
    }
    if (mCollection == nullptr) {
        SetCollection(CollectionT());
        // If calling Process() didn't result in a call to Set() or SetCollection(), we create an empty collection
        // so that podio::ROOTWriter doesn't segfault on the null mCollection pointer
    }
}

template <typename T>
void JFactoryPodioT<T>::Set(const std::vector<T*>& aData) {
    CollectionT collection;
    if (mIsSubsetCollection) collection.setSubsetCollection(true);
    for (T* item : aData) {
        collection.push_back(*item);
        delete item;
    }
    SetCollection(std::move(collection));
}

template <typename T>
void JFactoryPodioT<T>::Set(std::vector<T*>&& aData) {
    CollectionT collection;
    if (mIsSubsetCollection) collection.setSubsetCollection(true);
    for (T* item : aData) {
        collection.push_back(*item);
        delete item;
    }
    SetCollection(std::move(collection));
}

template <typename T>
void JFactoryPodioT<T>::Insert(T* aDatum) {
    CollectionT collection;
    if (mIsSubsetCollection) collection->setSubsetCollection(true);
    collection->push_back(*aDatum);
    delete aDatum;
    SetCollection(std::move(collection));
}

