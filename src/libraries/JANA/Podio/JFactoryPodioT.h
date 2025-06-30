
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include <JANA/JFactory.h>
#include <JANA/Components/JPodioOutput.h>
#include <podio/Frame.h>

/// The point of this additional base class is to allow us _untyped_ access to the underlying PODIO collection,
/// at the cost of some weird multiple inheritance. The JEvent can trigger the untyped factory using Create(), then
///
class JFactoryPodio {
protected:
    const podio::CollectionBase* mCollection = nullptr;
    bool mIsSubsetCollection = false;
    podio::Frame* mFrame = nullptr;

public:
    // Meant to be called from JEvent or VariadicPodioInput
    const podio::CollectionBase* GetCollection() { return mCollection; }
    void SetFrame(podio::Frame* frame) { mFrame = frame; }

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
    jana::components::PodioOutput<T> mOutput {this};

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
    void Create(const JEvent& event) final;

    void SetTag(std::string tag) { mOutput.SetUniqueName(tag); }

    std::type_index GetObjectType() const final { return std::type_index(typeid(T)); }
    std::size_t GetNumObjects() const final { return mOutput.GetDatabundle()->GetSize(); }
    void ClearData() final;

    void SetCollection(CollectionT&& collection);
    void SetCollection(std::unique_ptr<CollectionT> collection);


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
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection.

    CollectionT* coll = new CollectionT;
    *coll = std::move(collection);
    this->mOutput.GetDatabundle()->SetCollection(coll);
    // Inserting into frame is handled by StoreData()
    // Updating JFactoryPodio::mCollection, mFrame is handled by Create()
}


template <typename T>
void JFactoryPodioT<T>::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    CollectionT* coll = collection.release();
    this->mOutput.GetDatabundle()->SetCollection(coll);
    // Inserting into frame is handled by StoreData()
    // Updating JFactoryPodio::mCollection, mFrame is handled by Create()
}


template <typename T>
void JFactoryPodioT<T>::ClearData() {
    if (this->mStatus == JFactory::Status::Uninitialized) {
        return;
    }

    this->mStatus = JFactory::Status::Unprocessed;
    this->mCreationStatus = JFactory::CreationStatus::NotCreatedYet;

    for (auto* output : this->GetDatabundleOutputs()) {
        for (auto* db : output->GetDatabundles()) {
            db->ClearData();
        }
    }
    mCollection = nullptr;
    mFrame = nullptr;
}

template <typename T>
void JFactoryPodioT<T>::SetCollectionAlreadyInFrame(const CollectionT* collection) {
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
    this->mOutput.GetDatabundle()->SetCollection(collection);
}

// This free function is used to break the dependency loop between JFactoryPodioT and JEvent.
podio::Frame* GetOrCreateFrame(const JEvent& event);

template <typename T>
void JFactoryPodioT<T>::Create(const JEvent& event) {
    auto* frame = GetOrCreateFrame(event);
    try {
        JFactory::Create(event);
        mFrame = mOutput.GetDatabundle()->GetFrame();
        mCollection = mOutput.GetDatabundle()->GetCollection();
    }
    catch (...) {
        if (mOutput.GetDatabundle()->GetCollection() == nullptr) {
            // If calling Create() excepts, we still create an empty collection
            // so that podio::ROOTWriter doesn't segfault on the null mCollection pointer
            const std::string& unique_name = mOutput.GetDatabundle()->GetUniqueName();
            frame->put(CollectionT(), unique_name);
            const auto* moved = &frame->template get<CollectionT>(unique_name);
            mOutput.GetDatabundle()->SetCollection(moved);
            mOutput.GetDatabundle()->SetFrame(frame);
            mCollection = moved;
            mFrame = frame;
        }
        throw;
    }
}

template <typename T>
void JFactoryPodioT<T>::Create(const std::shared_ptr<const JEvent>& event) {
    Create(*event);
}


