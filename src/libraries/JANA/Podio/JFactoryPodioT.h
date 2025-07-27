
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include <JANA/JFactory.h>
#include <JANA/Components/JPodioOutput.h>
#include <podio/Frame.h>


template <typename T>
class JFactoryPodioT : public JFactory {
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
    void SetSubsetCollection(bool is_subset=true) { mOutput.SetSubsetCollection(is_subset); }

    void SetCollection(CollectionT&& collection);
    void SetCollection(std::unique_ptr<CollectionT> collection);
    const podio::CollectionBase* GetCollection() { return mOutput.GetDatabundle()->GetCollection(); }


private:
    // This is meant to be called by JEvent::Insert
    friend class JEvent;
    void SetCollectionAlreadyInFrame(const CollectionT* collection);

};


template <typename T>
JFactoryPodioT<T>::JFactoryPodioT() {
    SetObjectName(JTypeInfo::demangle<T>());
}

template <typename T>
JFactoryPodioT<T>::~JFactoryPodioT() {
    // Ownership of mData, mCollection, and mFrame is complicated, so we always handle it via ClearData()
    ClearData();
}

template <typename T>
void JFactoryPodioT<T>::SetCollection(CollectionT&& collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection.

    (*mOutput()) = std::move(collection);
}


template <typename T>
void JFactoryPodioT<T>::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    mOutput() = std::move(collection);
}


template <typename T>
void JFactoryPodioT<T>::SetCollectionAlreadyInFrame(const CollectionT* collection) {
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
    this->mOutput.GetDatabundle()->SetCollection(collection);
    this->mOutput.GetDatabundle()->SetStatus(JDatabundle::Status::Inserted);
}

// This free function is used to break the dependency loop between JFactoryPodioT and JEvent.
podio::Frame* GetOrCreateFrame(const JEvent& event);

template <typename T>
void JFactoryPodioT<T>::Create(const JEvent& event) {
    try {
        JFactory::Create(event);
    }
    catch (...) {
        if (mOutput.GetDatabundle()->GetCollection() == nullptr) {
            // If calling Create() excepts, we still create an empty collection
            // so that podio::ROOTWriter doesn't segfault on the null mCollection pointer
            const std::string& unique_name = mOutput.GetDatabundle()->GetUniqueName();
            auto* frame = GetOrCreateFrame(event);
            auto coll = CollectionT();
            coll.setSubsetCollection(mOutput.IsSubsetCollection());
            frame->put(CollectionT(), unique_name);
            const auto* moved = &frame->template get<CollectionT>(unique_name);
            mOutput.GetDatabundle()->SetCollection(moved);
        }
        throw;
    }
}

template <typename T>
void JFactoryPodioT<T>::Create(const std::shared_ptr<const JEvent>& event) {
    Create(*event);
}


