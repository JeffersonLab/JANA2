
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JFACTORYPODIOT_H
#define JANA2_JFACTORYPODIOT_H

#include <JANA/JFactory.h>
#include <podio/Frame.h>

template <typename S> struct PodioTypeMap;

template <typename T>
class JFactoryPodioT : public JFactory {
    // mCollection is owned by the frame.
    // mFrame is owned by the JFactoryT<podio::Frame>.
    // mData holds lightweight value objects which hold a pointer into mCollection.
    // This factory owns these value objects.
    // podio::Frame* mFrame = nullptr;
    const typename PodioTypeMap<T>::collection_t* mCollection = nullptr;
    std::vector<const T*> mData;

public:

    explicit JFactoryPodioT() : JFactory(JTypeInfo::demangle<T>(), "") {};
    ~JFactoryPodioT() override = default;

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void ChangeRun(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}

    std::type_index GetObjectType() const override { return std::type_index(typeid(T)); }
    std::size_t GetNumObjects() const override { return mCollection->size(); }
    void ClearData() override;

    const podio::CollectionBase* GetCollection() { return mCollection; }
    void SetCollection(const std::shared_ptr<const JEvent>&, std::unique_ptr<typename PodioTypeMap<T>::collection_t> collection);

};

podio::Frame* GetFrame(const JEvent& event);

template <typename T>
void JFactoryPodioT<T>::SetCollection(const std::shared_ptr<const JEvent> & event,
                                      std::unique_ptr<typename PodioTypeMap<T>::collection_t> collection) {

    // auto moved = mFrame->put(collection, mTag);
    auto frame = GetFrame(*event);
    auto moved = frame->put(collection, mTag);
    mCollection = &moved;
    for (const T& item : moved) {
        mData.push_back(&item);
    }
    mStatus = Status::Inserted;
    mCreationStatus = CreationStatus::Inserted;
}

template <typename T>
void JFactoryPodioT<T>::ClearData() {
    for (auto p : mData) delete p;
    mData.clear();
    mCollection = nullptr;  // Collection is owned by the Frame, so we ignore here
    mFrame = nullptr;  // Frame is owned by the JEvent, so we ignore here
    mStatus = Status::Unprocessed;
    mCreationStatus = CreationStatus::NotCreatedYet;
}



#endif //JANA2_JFACTORYPODIOT_H
