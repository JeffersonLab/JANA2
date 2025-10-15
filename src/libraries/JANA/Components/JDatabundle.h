// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Utils/JAny.h>
#include <JANA/Utils/JCallGraphRecorder.h>

#include <string>
#include <functional>
#include <typeindex>
#include <vector>
#include <memory>
#include <unordered_map>


class JFactory;

class JDatabundle {
public:
    // Typedefs
    enum class Status { Empty, Created, Inserted, Excepted };
    struct NoTypeProvided {}; // This gives us a default value for m_type_index

private:
    // Fields
    Status m_status = Status::Empty;
    std::string m_unique_name;
    std::string m_short_name;
    bool m_has_short_name = true;
    std::string m_type_name;
    JFactory* m_factory = nullptr;
    std::type_index m_inner_type_index = std::type_index(typeid(NoTypeProvided));
    mutable JCallGraphRecorder::JDataOrigin m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE;


protected:
    std::shared_ptr<std::unordered_map<std::type_index, std::unique_ptr<JAny>>> m_upcast_fns;

public:
    // Interface
    JDatabundle() {
        m_upcast_fns = std::make_shared<std::unordered_map<std::type_index, std::unique_ptr<JAny>>>();
    }
    JDatabundle(const JDatabundle& other) {
        m_status = other.m_status;
        m_unique_name = other.m_unique_name;
        m_short_name = other.m_short_name;
        m_type_name = other.m_type_name;
        m_factory = nullptr;
        // We do NOT propagate m_factory because JFactorySet assumes that
        // m_factory is this object's owner.

        m_inner_type_index = other.m_inner_type_index;
        m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE;
        m_upcast_fns = other.m_upcast_fns;
        // We use shared_ptr for upcast functions so that we can
        // call EnableGetAs<> once from a JEventSource or JEventUnfolder ctor,
        // and GetAs<> from JEventProcessor::Process succeeds
    }
    virtual ~JDatabundle() = default;
    virtual size_t GetSize() const = 0;
    virtual void ClearData() = 0;

    // Getters
    Status GetStatus() const { return m_status; }
    std::string GetUniqueName() const { return m_unique_name; }
    std::string GetShortName() const { return m_short_name; }
    bool HasShortName() const { return m_has_short_name; }
    std::string GetTypeName() const { return m_type_name; }
    std::type_index GetTypeIndex() const { return m_inner_type_index; }
    JCallGraphRecorder::JDataOrigin GetInsertOrigin() const { return m_insert_origin; } ///< If objects were placed here by JEvent::Insert() this records whether that call was made from a source or factory.
    JFactory* GetFactory() const { return m_factory; }

    // Setters
    void SetStatus(Status s) { m_status = s;}
    void SetUniqueName(std::string unique_name);
    void SetShortName(std::string short_name);
    void SetTypeName(std::string type_name);
    void SetTypeIndex(std::type_index index) { m_inner_type_index = index; }
    void SetInsertOrigin(JCallGraphRecorder::JDataOrigin origin) { m_insert_origin = origin; } ///< Called automatically by JEvent::Insert() to records whether that call was made by a source or factory.
    void SetFactory(JFactory* fac) { m_factory = fac; }

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



// Because C++ doesn't support templated virtual functions, we implement our own dispatch table, m_upcast_fns.
// This means that the JFactoryT is forced to manually populate this table by calling JFactoryT<T>::EnableGetAs.
// We have the option to make the vtable be a static member of JFactoryT<T>, but we have chosen not to because:
//
//   1. It would be inconsistent with the fact that the user is supposed to call EnableGetAs in the ctor
//   2. People in the future may want to generalize GetAs to support user-defined S* -> T* conversions (which I don't recommend)
//   3. The size of the vtable is expected to be very small (<10 elements, most likely 2)

template<typename S>
std::vector<S*> JDatabundle::GetAs() {
    std::vector<S*> results;
    auto ti = std::type_index(typeid(S));
    auto search = m_upcast_fns->find(ti);
    if (search != m_upcast_fns->end()) {
        using upcast_fn_t = std::function<std::vector<S*>()>;
        auto temp = static_cast<JAnyT<upcast_fn_t>*>(&(*search->second));
        upcast_fn_t upcast_fn = temp->t;
        results = upcast_fn();
    }
    return results;
}


