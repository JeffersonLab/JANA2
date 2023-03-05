
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JFACTORYPODIOT_H
#define JANA2_JFACTORYPODIOT_H

#include <JANA/JFactoryT.h>
#include <podio/Frame.h>

template <typename S> struct PodioTypeMap;

/// The point of this additional base class is to allow us _untyped_ access to the underlying PODIO collection,
/// at the cost of some weird multiple inheritance. The JEvent can trigger the untyped factory using Create(), then
///
class JFactoryPodio {
protected:
    const podio::CollectionBase* mCollection = nullptr;
private:
    friend class JEvent;
    const podio::CollectionBase* GetCollection() { return mCollection; }
};


template <typename T>
class JFactoryPodioT : public JFactoryT<T>, public JFactoryPodio {
public:
    using CollectionT = typename PodioTypeMap<T>::collection_t;
private:
    // mCollection is owned by the frame.
    // mFrame is owned by the JFactoryT<podio::Frame>.
    // mData holds lightweight value objects which hold a pointer into mCollection.
    // This factory owns these value objects.
    // podio::Frame* mFrame = nullptr;
    podio::Frame* mFrame = nullptr;

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

    void SetCollection(CollectionT* collection);
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
void JFactoryPodioT<T>::SetCollection(typename PodioTypeMap<T>::collection_t* collection) {

    if (this->mFrame == nullptr) {
        throw JException("JFactoryPodioT: Unable to add collection to frame as frame is missing!");
    }
    auto& moved = this->mFrame->put(std::move(*collection), this->GetTag());
    this->mCollection = &moved;
    for (const T& item : moved) {
        T* clone = new T(item);
        this->mData.push_back(clone); // TODO: Verify that clone points to underlying and does not do a deep copy
    }
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}

template <typename T>
void JFactoryPodioT<T>::ClearData() {
    for (auto p : this->mData) delete p;
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
        this->mData.push_back(clone); // TODO: Verify that clone points to underlying and does not do a deep copy
    }
    this->mCollection = collection;
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}

// This free function is used to break the dependency loop between JFactoryPodioT and JEvent.
podio::Frame*GetOrCreateFrame(const std::shared_ptr<const JEvent>& event);

template <typename T>
void JFactoryPodioT<T>::Create(const std::shared_ptr<const JEvent>& event) {
    mFrame = GetOrCreateFrame(event);
    JFactory::Create(event);
}

template <typename T>
void JFactoryPodioT<T>::Set(const std::vector<T*>& aData) {
    auto* collection = new typename PodioTypeMap<T>::collection_t();
    for (T* item : aData) {
        collection->push_back(*item);
    }
    SetCollection(collection);
}

template <typename T>
void JFactoryPodioT<T>::Set(std::vector<T*>&& aData) {
    auto* collection = new typename PodioTypeMap<T>::collection_t();
    for (T* item : aData) {
        collection->push_back(*item);
    }
    SetCollection(collection);
}

template <typename T>
void JFactoryPodioT<T>::Insert(T* aDatum) {
    auto* collection = new typename PodioTypeMap<T>::collection_t();
    collection->push_back(*aDatum);
    SetCollection(collection);
}

#endif //JANA2_JFACTORYPODIOT_H
