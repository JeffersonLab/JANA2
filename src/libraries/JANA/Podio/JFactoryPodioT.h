
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once

#include <JANA/JFactoryT.h>
#include <JANA/Components/JPodioOutput.h>


template <typename T>
class JFactoryPodioT : public JFactoryT<T> {
public:
    using CollectionT = typename T::collection_type;

private:
    jana::components::PodioOutput<T> m_output {this};

public:
    explicit JFactoryPodioT();
    ~JFactoryPodioT() override;

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void ChangeRun(const std::shared_ptr<const JEvent>&) override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Finish() override {}

    std::optional<std::type_index> GetObjectType() const final { return std::type_index(typeid(T)); }
    std::size_t GetNumObjects() const final { return m_output.GetCollection()->GetSize(); }
    void ClearData() final;

    void SetCollection(CollectionT&& collection);
    void SetCollection(std::unique_ptr<CollectionT> collection);
    void Set(const std::vector<T*>& aData) final;
    void Set(std::vector<T*>&& aData) final;
    void Insert(T* aDatum) final;

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

    m_output() = std::make_unique<CollectionT>(std::move(collection));
}


template <typename T>
void JFactoryPodioT<T>::SetCollection(std::unique_ptr<CollectionT> collection) {
    /// Provide a PODIO collection. Note that PODIO assumes ownership of this collection, and the
    /// collection pointer should be assumed to be invalid after this call

    m_output() = std::move(collection);
}


template <typename T>
void JFactoryPodioT<T>::ClearData() {
    // Happens automatically now
}

template <typename T>
void JFactoryPodioT<T>::Set(const std::vector<T*>&) {
    throw JException("Not supported!");
}

template <typename T>
void JFactoryPodioT<T>::Set(std::vector<T*>&&) {
    throw JException("Not supported!");
}

template <typename T>
void JFactoryPodioT<T>::Insert(T*) {
    throw JException("Not supported!");
}

