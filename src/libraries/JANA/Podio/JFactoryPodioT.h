
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
    std::vector<const T*> mData;

public:
    explicit JFactoryPodioT(std::string tag);
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
    void SetCollection(const std::shared_ptr<const JEvent>&, std::unique_ptr<typename PodioTypeMap<T>::collection_t> collection);

};

// This helper function is needed to break the dependency loop between JFactory and JEvent.
podio::Frame* GetFrame(const JEvent& event);


template <typename T>
JFactoryPodioT<T>::JFactoryPodioT(std::string tag) : JFactory(JTypeInfo::demangle<T>(), tag) {
    if (tag.empty())
        throw std::runtime_error("PODIO factories require a unique, non-empty tag because the tag is also used as the PODIO collection name");
}

template <typename T>
JFactoryPodioT<T>::~JFactoryPodioT() {
    // Ownership of mData, mCollection, and mFrame is complicated, so we always handle it via ClearData()
    ClearData();
}

template <typename T>
void JFactoryPodioT<T>::SetCollection(const std::shared_ptr<const JEvent> & event,
                                      std::unique_ptr<typename PodioTypeMap<T>::collection_t> collection) {

    // auto moved = mFrame->put(collection, mTag);
    auto frame = GetFrame(*event);
    auto moved = frame->put(collection, this->GetTag());
    mCollection = &moved;
    for (const T& item : moved) {
        mData.push_back(&item);
    }
    this->mStatus = JFactory::Status::Inserted;
    this->mCreationStatus = JFactory::CreationStatus::Inserted;
}

template <typename T>
void JFactoryPodioT<T>::ClearData() {
    for (auto p : mData) delete p;
    mData.clear();
    mCollection = nullptr;  // Collection is owned by the Frame, so we ignore here
    this->mFrame = nullptr;  // Frame is owned by the JEvent, so we ignore here
    this->mStatus = JFactory::Status::Unprocessed;
    this->mCreationStatus = JFactory::CreationStatus::NotCreatedYet;
}



#endif //JANA2_JFACTORYPODIOT_H
