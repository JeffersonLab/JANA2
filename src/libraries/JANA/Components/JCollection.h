// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JException.h>
#include <JANA/Utils/JAny.h>
#include <JANA/Utils/JCallGraphRecorder.h>

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>


class JCollection {
public:
    // Typedefs
    enum class CreationStatus { NotCreatedYet, Created, Inserted, InsertedViaGetObjects, NeverCreated };

private:
    // Fields
    CreationStatus m_creation_status = CreationStatus::NotCreatedYet;
    std::string m_collection_name;
    std::string m_collection_tag;
    std::string m_type_name;
    mutable JCallGraphRecorder::JDataOrigin m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE;

protected:
    std::unordered_map<std::type_index, std::unique_ptr<JAny>> mUpcastVTable;

public:
    // Interface
    JCollection() = default;
    virtual ~JCollection() = default;
    virtual size_t GetSize() const = 0;
    virtual void ClearData() = 0;

    // Getters
    CreationStatus GetCreationStatus() { return m_creation_status; }
    std::string GetCollectionName() { return m_collection_name; }
    std::string GetCollectionTag() { return m_collection_tag; }
    std::string GetTypeName() { return m_type_name; }
    JCallGraphRecorder::JDataOrigin GetInsertOrigin() const { return m_insert_origin; } ///< If objects were placed here by JEvent::Insert() this records whether that call was made from a source or factory.

    // Setters
    void SetCreationStatus(CreationStatus s) { m_creation_status = s;}
    void SetCollectionName(std::string collection_name) { m_collection_name = collection_name; }
    void SetCollectionTag(std::string collection_tag) { m_collection_tag = collection_tag; }
    void SetTypeName(std::string type_name) { m_type_name = type_name; }
    void SetInsertOrigin(JCallGraphRecorder::JDataOrigin origin) { m_insert_origin = origin; } ///< Called automatically by JEvent::Insert() to records whether that call was made by a source or factory.
                                                                                               ///
    // Templates 
    //
    /// Generically access the encapsulated data, performing an upcast if necessary. This is useful for extracting data from
    /// all JFactories<T> where T extends a parent class S, such as JObject or TObject, in contexts where T is not known
    /// or it would introduce an unwanted coupling. The main application is for building DSTs.
    ///
    /// Be aware of the following caveats:
    /// - The factory's object type must not use virtual inheritance.
    /// - If JFactory::Process hasn't already been called, this will return an empty vector. This will NOT call JFactory::Process.
    /// - Someone must call JFactoryT<T>::EnableGetAs<S>, preferably the constructor. Otherwise, this will return an empty vector.
    /// - If S isn't a base class of T, this will return an empty vector.
    template<typename S>
    std::vector<S*> GetAs();

};



// Because C++ doesn't support templated virtual functions, we implement our own dispatch table, mUpcastVTable.
// This means that the JFactoryT is forced to manually populate this table by calling JFactoryT<T>::EnableGetAs.
// We have the option to make the vtable be a static member of JFactoryT<T>, but we have chosen not to because:
//
//   1. It would be inconsistent with the fact that the user is supposed to call EnableGetAs in the ctor
//   2. People in the future may want to generalize GetAs to support user-defined S* -> T* conversions (which I don't recommend)
//   3. The size of the vtable is expected to be very small (<10 elements, most likely 2)

template<typename S>
std::vector<S*> JCollection::GetAs() {
    std::vector<S*> results;
    auto ti = std::type_index(typeid(S));
    auto search = mUpcastVTable.find(ti);
    if (search != mUpcastVTable.end()) {
        using upcast_fn_t = std::function<std::vector<S*>()>;
        auto temp = static_cast<JAnyT<upcast_fn_t>*>(&(*search->second));
        upcast_fn_t upcast_fn = temp->t;
        results = upcast_fn();
    }
    return results;
}


