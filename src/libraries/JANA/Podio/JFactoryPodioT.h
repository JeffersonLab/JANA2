
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JFACTORYPODIOT_H
#define JANA2_JFACTORYPODIOT_H

#include <JANA/JFactoryT.h>
#include <podio/Frame.h>

template <typename S> struct PodioTypeMap;

template <typename T>
class JFactoryPodioT : public JFactoryT<T> {
    // mCollection is owned by the frame.
    // mFrame is owned by the JFactoryT<podio::Frame>.
    // mData holds lightweight value objects which hold a pointer into mCollection.
    // This factory owns these value objects.
    // podio::Frame* mFrame = nullptr;
    const typename PodioTypeMap<T>::collection_t* mCollection = nullptr;

public:
    explicit JFactoryPodioT();
    ~JFactoryPodioT() override;

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void ChangeRun(const std::shared_ptr<const JEvent>&) override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Finish() override {}

    std::type_index GetObjectType() const final { return std::type_index(typeid(T)); }
    std::size_t GetNumObjects() const final { return mCollection->size(); }
    void ClearData() final;

    const typename PodioTypeMap<T>::collection_t* GetCollection() { return mCollection; }
    void SetCollection(const std::shared_ptr<const JEvent>&, typename PodioTypeMap<T>::collection_t* collection);

};

// This helper function is needed to break the dependency loop between JFactory and JEvent.
podio::Frame* GetFrame(const JEvent& event);


template <typename T>
JFactoryPodioT<T>::JFactoryPodioT() : JFactoryT<T>(JTypeInfo::demangle<T>(), "") {}

template <typename T>
JFactoryPodioT<T>::~JFactoryPodioT() {
    // Ownership of mData, mCollection, and mFrame is complicated, so we always handle it via ClearData()
    ClearData();
}

template <typename T>
void JFactoryPodioT<T>::SetCollection(const std::shared_ptr<const JEvent> & event,
                                      typename PodioTypeMap<T>::collection_t* collection) {

    // auto moved = mFrame->put(collection, mTag);
    auto frame = GetFrame(*event);
    auto& moved = frame->put(std::move(*collection), this->GetTag());
    mCollection = &moved;
    for (const T& item : moved) {
        this->mData.push_back(&item);
    }
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}

template <typename T>
void JFactoryPodioT<T>::ClearData() {
    for (auto p : this->mData) delete p;
    this->mData.clear();
    mCollection = nullptr;  // Collection is owned by the Frame, so we ignore here
    this->mFrame = nullptr;  // Frame is owned by the JEvent, so we ignore here
    this->mStatus = JFactory::Status::Unprocessed;
    this->mCreationStatus = JFactory::CreationStatus::NotCreatedYet;
}



#endif //JANA2_JFACTORYPODIOT_H
