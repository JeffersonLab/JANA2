
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JObject.h>
#include <JANA/JException.h>
#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>
#include <JANA/JLogger.h>
#include <JANA/JVersion.h>

#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Utils/JCallGraphRecorder.h>
#include <JANA/Utils/JCallGraphEntryMaker.h>
#include <JANA/Utils/JInspector.h>

#include <vector>
#include <cstddef>
#include <memory>
#include <atomic>

#if JANA2_HAVE_PODIO
#include <JANA/Podio/JFactoryPodioT.h>
namespace podio {
class CollectionBase;
}
#endif

class JApplication;
class JEventSource;


class JEvent : public std::enable_shared_from_this<JEvent> {

private:
    JApplication* mApplication = nullptr;
    int32_t mRunNumber = 0;
    uint64_t mEventNumber = 0;
    mutable JFactorySet* mFactorySet = nullptr;
    mutable JCallGraphRecorder mCallGraph;
    mutable JInspector mInspector;
    bool mUseDefaultTags = false;
    std::map<std::string, std::string> mDefaultTags;
    JEventSource* mEventSource = nullptr;
    bool mIsBarrierEvent = false;
    bool mIsWarmedUp = false;

    // Hierarchical event memory management
    std::vector<std::pair<JEventLevel, JEvent*>> mParents;
    std::atomic_int mReferenceCount {1};
    int64_t mEventIndex = -1;

#if JANA2_HAVE_PODIO
    std::map<std::string, JFactory*> mPodioFactories;
#endif


public:
    JEvent();
    explicit JEvent(JApplication* app);
    virtual ~JEvent();

    void SetFactorySet(JFactorySet* aFactorySet);
    void SetRunNumber(int32_t aRunNumber){mRunNumber = aRunNumber;}
    void SetEventNumber(uint64_t aEventNumber){mEventNumber = aEventNumber;}
    void SetJApplication(JApplication* app){mApplication = app;}
    void SetJEventSource(JEventSource* aSource){mEventSource = aSource;}
    void SetDefaultTags(std::map<std::string, std::string> aDefaultTags){mDefaultTags=aDefaultTags; mUseDefaultTags = !mDefaultTags.empty();}
    void SetSequential(bool isSequential) {mIsBarrierEvent = isSequential;}

    JFactorySet* GetFactorySet() const { return mFactorySet; }
    int32_t GetRunNumber() const {return mRunNumber;}
    uint64_t GetEventNumber() const {return mEventNumber;}
    JApplication* GetJApplication() const {return mApplication;}
    JEventSource* GetJEventSource() const {return mEventSource; }
    JCallGraphRecorder* GetJCallGraphRecorder() const {return &mCallGraph;}
    JInspector* GetJInspector() const {return &mInspector;}
    void Inspect() const { mInspector.Loop();}
    bool GetSequential() const {return mIsBarrierEvent;}
    bool IsWarmedUp() { return mIsWarmedUp; }

    // Hierarchical
    JEventLevel GetLevel() const { return mFactorySet->GetLevel(); }
    void SetLevel(JEventLevel level) { mFactorySet->SetLevel(level); }
    void SetEventIndex(int event_index) { mEventIndex = event_index; }
    int64_t GetEventIndex() const { return mEventIndex; }

    bool HasParent(JEventLevel level) const;
    const JEvent& GetParent(JEventLevel level) const;
    void SetParent(JEvent* parent);
    JEvent* ReleaseParent(JEventLevel level);
    void Release();

    // Lifecycle
    void Clear();
    void Finish();

    JFactory* GetFactory(const std::string& object_name, const std::string& tag) const;
    std::vector<JFactory*> GetAllFactories() const;


    template<class T> JFactoryT<T>* GetFactory(const std::string& tag = "", bool throw_on_missing=false) const;
    template<class T> std::vector<JFactoryT<T>*> GetFactoryAll(bool throw_on_missing = false) const;

    // C style getters
    template<class T> JFactoryT<T>* GetSingle(const T* &t, const char *tag="", bool exception_if_not_one=true) const;
    template<class T> JFactoryT<T>* Get(const T** item, const std::string& tag="") const;
    template<class T> JFactoryT<T>* Get(std::vector<const T*> &vec, const std::string& tag = "", bool strict=true) const;
    template<class T> void GetAll(std::vector<const T*> &vec) const;

    // C++ style getters
    template<class T> const T* GetSingle(const std::string& tag = "") const;
    template<class T> const T* GetSingleStrict(const std::string& tag = "") const;
    template<class T> std::vector<const T*> Get(const std::string& tag = "", bool strict=true) const;
    template<class T> typename JFactoryT<T>::PairType GetIterators(const std::string& aTag = "") const;
    template<class T> std::vector<const T*> GetAll() const;
    template<class T> std::map<std::pair<std::string,std::string>,std::vector<T*>> GetAllChildren() const;

    // Insert
    template <class T> JFactoryT<T>* Insert(T* item, const std::string& aTag = "") const;
    template <class T> JFactoryT<T>* Insert(const std::vector<T*>& items, const std::string& tag = "") const;

    // PODIO
#if JANA2_HAVE_PODIO
    std::vector<std::string> GetAllCollectionNames() const;
    const podio::CollectionBase* GetCollectionBase(std::string name, bool throw_on_missing=true) const;
    template <typename T> const typename JFactoryPodioT<T>::CollectionT* GetCollection(std::string name, bool throw_on_missing=true) const;
    template <typename T> JFactoryPodioT<T>* InsertCollection(typename JFactoryPodioT<T>::CollectionT&& collection, std::string name);
    template <typename T> JFactoryPodioT<T>* InsertCollectionAlreadyInFrame(const podio::CollectionBase* collection, std::string name);
#endif


};


/// GetFactory() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory.
template<class T>
inline JFactoryT<T>* JEvent::GetFactory(const std::string& tag, bool throw_on_missing) const
{
    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(JTypeInfo::demangle<T>());
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    auto factory = mFactorySet->GetFactory<T>(resolved_tag);
    if (factory == nullptr) {
        if (throw_on_missing) {
            JException ex("Could not find JFactoryT<" + JTypeInfo::demangle<T>() + "> with tag=" + tag);
            ex.show_stacktrace = false;
            throw ex;
        }
    };
    return factory;
}


/// GetFactoryAll returns all JFactoryT's for type T (each corresponds to a different tag).
/// This is useful when there are many different tags, or the tags are unknown, and the user
/// wishes to examine them all together.
template<class T>
inline std::vector<JFactoryT<T>*> JEvent::GetFactoryAll(bool throw_on_missing) const {
    auto factories = mFactorySet->GetAllFactories<T>();
    if (factories.size() == 0) {
        if (throw_on_missing) {
            JException ex("Could not find any JFactoryT<" + JTypeInfo::demangle<T>() + "> (from any tag)");
            ex.show_stacktrace = false;
            throw ex;
        }
    };
    return factories;
}


/// C-style getters

template<class T>
JFactoryT<T>* JEvent::GetSingle(const T* &t, const char *tag, bool exception_if_not_one) const
{
    /// This is a convenience method that can be used to get a pointer to the single
    /// object of type T from the specified factory. It simply calls the Get(vector<...>) method
    /// and copies the first pointer into "t" (or NULL if something other than 1 object is returned).
    ///
    /// This is intended to address the common situation in which there is an interest
    /// in the event if and only if there is exactly 1 object of type T. If the event
    /// has no objects of that type or more than 1 object of that type (for the specified
    /// factory) then an exception of type "unsigned long" is thrown with the value
    /// being the number of objects of type T. You can supress the exception by setting
    /// exception_if_not_one to false. In that case, you will have to check if t==NULL to
    /// know if the call succeeded.

    std::vector<const T*> v;
    auto fac = GetFactory<T>(tag, true); // throw exception if factory not found
    JCallGraphEntryMaker cg_entry(mCallGraph, fac); // times execution until this goes out of scope
    Get(v, tag);
    if(v.size()!=1){
        t = NULL;
        if(exception_if_not_one) throw v.size();
    }
    t = v[0];
    return fac;
}

/// Get conveniently returns one item from inside the JFactory. This should be used when the data in question
/// is optional and the caller wants to examine the result and decide how to proceed. The caller should embed this
/// inside an if-block. Get updates the `destination` out parameter and returns a pointer to the enclosing JFactory.
/// - If the factory is missing, GetSingle throws an exception.
/// - If the factory exists but contains no items, GetSingle updates the `destination` to point to nullptr.
/// - If the factory contains exactly one item, GetSingle updates the `destination` to point to that item.
/// - If the factory contains more than one item, GetSingle updates the `destination` to point to the first time.
template<class T>
JFactoryT<T>* JEvent::Get(const T** destination, const std::string& tag) const
{
    auto factory = GetFactory<T>(tag, true);
    JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
    auto iterators = factory->CreateAndGetData(this->shared_from_this());
    if (std::distance(iterators.first, iterators.second) == 0) {
        *destination = nullptr;
    }
    else {
        *destination = *iterators.first;
    }
    return factory;
}


template<class T>
JFactoryT<T>* JEvent::Get(std::vector<const T*>& destination, const std::string& tag, bool strict) const
{
    auto factory = GetFactory<T>(tag, strict);
    if (factory == nullptr) return nullptr; // Will have thrown already if strict==true
    JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
    auto iterators = factory->CreateAndGetData(this->shared_from_this());
    for (auto it=iterators.first; it!=iterators.second; it++) {
        destination.push_back(*it);
    }
    return factory;
}


/// GetAll returns all JObjects of (child) type T, regardless of tag.
template<class T>
void JEvent::GetAll(std::vector<const T*>& destination) const {
    auto factories = GetFactoryAll<T>(true);
    for (auto factory : factories) {
        auto iterators = factory->CreateAndGetData(this->shared_from_this());
        for (auto it = iterators.first; it != iterators.second; it++) {
            destination.push_back(*it);
        }
    }
}



/// C++ style getters

/// GetSingle conveniently returns one item from inside the JFactory. This should be used when the data in question
/// is optional and the caller wants to examine the result and decide how to proceed. The caller should embed this
/// inside an if-block.
/// - If the factory is missing, GetSingle throws an exception
/// - If the factory exists but contains no items, GetSingle returns nullptr
/// - If the factory contains more than one item, GetSingle returns the first item

template<class T> const T* JEvent::GetSingle(const std::string& tag) const {
    auto factory = GetFactory<T>(tag, true);
    JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
    auto iterators = factory->CreateAndGetData(this->shared_from_this());
    if (std::distance(iterators.first, iterators.second) == 0) {
        return nullptr;
    }
    return *iterators.first;
}



/// GetSingleStrict conveniently returns one item from inside the JFactory. This should be used when the data in
/// question is mandatory, and its absence indicates an error which should stop execution. The caller does not need
/// to embed this in an if- or try-catch block; it can be a one-liner.
/// - If the factory is missing, GetSingleStrict throws an exception
/// - If the factory exists but contains no items, GetSingleStrict throws an exception
/// - If the factory contains more than one item, GetSingleStrict throws an exception
template<class T> const T* JEvent::GetSingleStrict(const std::string& tag) const {
    auto factory = GetFactory<T>(tag, true);
    JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
    auto iterators = factory->CreateAndGetData(this->shared_from_this());
    if (std::distance(iterators.first, iterators.second) == 0) {
        JException ex("GetSingle failed due to missing %d", NAME_OF(T));
        ex.show_stacktrace = false;
        throw ex;
    }
    else if (std::distance(iterators.first, iterators.second) > 1) {
        JException ex("GetSingle failed due to too many %d", NAME_OF(T));
        ex.show_stacktrace = false;
        throw ex;
    }
    return *iterators.first;
}


template<class T>
std::vector<const T*> JEvent::Get(const std::string& tag, bool strict) const {

    auto factory = GetFactory<T>(tag, strict);
    std::vector<const T*> vec;
    if (factory == nullptr) return vec; // Will have thrown already if strict==true
    JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
    auto iters = factory->CreateAndGetData(this->shared_from_this());
    for (auto it=iters.first; it!=iters.second; ++it) {
        vec.push_back(*it);
    }
    return vec; // Assumes RVO
}

template<class T>
typename JFactoryT<T>::PairType JEvent::GetIterators(const std::string& tag) const {
    auto factory = GetFactory<T>(tag, true);
    JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
    auto iters = factory->CreateAndGetData(this->shared_from_this());
    return iters;
}

/// GetAll returns all JObjects of (child) type T, regardless of tag.
template<class T>
std::vector<const T*> JEvent::GetAll() const {
    std::vector<const T*> vec;
    auto factories = GetFactoryAll<T>(true);

    for (auto factory : factories) {
        auto iters = factory->CreateAndGetData(this->shared_from_this());
        std::vector<const T*> vec;
        for (auto it = iters.first; it != iters.second; ++it) {
            vec.push_back(*it);
        }
    }
    return vec; // Assumes RVO
}

// GetAllChildren will furnish a map { (type_name,tag_name) : [BaseClass*] } containing all JFactoryT<T> data where
// T inherits from BaseClass. Note that this _won't_ compute any results (unlike GetAll) because this is meant for
// things like visualizing and persisting DSTs.
// TODO: This is conceptually inconsistent with GetAll. Reconcile.

template<class S>
std::map<std::pair<std::string, std::string>, std::vector<S*>> JEvent::GetAllChildren() const {
    std::map<std::pair<std::string, std::string>, std::vector<S*>> results;
    for (JFactory* factory : mFactorySet->GetAllFactories()) {
        auto val = factory->GetAs<S>();
        if (!val.empty()) {
            auto key = std::make_pair(factory->GetObjectName(), factory->GetTag());
            results.insert(std::make_pair(key, val));
        }
    }
    return results;
}



/// Insert() allows an EventSource to insert items directly into the JEvent,
/// removing the need for user-extended JEvents and/or JEventSource::GetObjects(...)
/// Repeated calls to Insert() will append to the previous data rather than overwrite it,
/// which saves the user from having to allocate a throwaway vector and requires less error handling.
template <class T>
inline JFactoryT<T>* JEvent::Insert(T* item, const std::string& tag) const {

    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(JTypeInfo::demangle<T>());
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    auto factory = mFactorySet->GetFactory<T>(resolved_tag);
    if (factory == nullptr) {
        factory = new JFactoryT<T>;
        factory->SetTag(tag);
        factory->SetLevel(mFactorySet->GetLevel());
        mFactorySet->Add(factory);
    }
    factory->Insert(item);
    factory->SetInsertOrigin( mCallGraph.GetInsertDataOrigin() ); // (see note at top of JCallGraphRecorder.h)
    return factory;
}

template <class T>
inline JFactoryT<T>* JEvent::Insert(const std::vector<T*>& items, const std::string& tag) const {

    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(JTypeInfo::demangle<T>());
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    auto factory = mFactorySet->GetFactory<T>(resolved_tag);
    if (factory == nullptr) {
        factory = new JFactoryT<T>;
        factory->SetTag(tag);
        factory->SetLevel(mFactorySet->GetLevel());
        mFactorySet->Add(factory);
    }
    for (T* item : items) {
        factory->Insert(item);
    }
    factory->SetStatus(JFactory::Status::Inserted); // for when items is empty
    factory->SetCreationStatus(JFactory::CreationStatus::Inserted); // for when items is empty
    factory->SetInsertOrigin( mCallGraph.GetInsertDataOrigin() ); // (see note at top of JCallGraphRecorder.h)
    return factory;
}



#if JANA2_HAVE_PODIO

inline std::vector<std::string> JEvent::GetAllCollectionNames() const {
    std::vector<std::string> keys;
    for (auto pair : mPodioFactories) {
        keys.push_back(pair.first);
    }
    return keys;
}

inline const podio::CollectionBase* JEvent::GetCollectionBase(std::string name, bool throw_on_missing) const {
    auto it = mPodioFactories.find(name);
    if (it == mPodioFactories.end()) {
        if (throw_on_missing) {
            throw JException("No factory with tag '%s' found", name.c_str());
        }
        else {
            return nullptr;
        }
    }
    JFactoryPodio* factory = dynamic_cast<JFactoryPodio*>(it->second);
    if (factory == nullptr) {
        // Should be no way to get here if we encapsulate mPodioFactories correctly
        throw JException("Factory with tag '%s' does not inherit from JFactoryPodio!", name.c_str());
    }
    JCallGraphEntryMaker cg_entry(mCallGraph, it->second); // times execution until this goes out of scope
    it->second->Create(this->shared_from_this());
    return factory->GetCollection();
}


template <typename T>
const typename JFactoryPodioT<T>::CollectionT* JEvent::GetCollection(std::string name, bool throw_on_missing) const {
    JFactoryT<T>* factory = GetFactory<T>(name, throw_on_missing);
    if (factory == nullptr) {
        return nullptr;
    }
    JFactoryPodioT<T>* typed_factory = dynamic_cast<JFactoryPodioT<T>*>(factory);
    if (typed_factory == nullptr) {
        throw JException("Factory must inherit from JFactoryPodioT in order to use JEvent::GetCollection()");
    }
    JCallGraphEntryMaker cg_entry(mCallGraph, typed_factory); // times execution until this goes out of scope
    typed_factory->Create(this->shared_from_this());
    return static_cast<const typename JFactoryPodioT<T>::CollectionT*>(typed_factory->GetCollection());
}


template <typename T>
JFactoryPodioT<T>* JEvent::InsertCollection(typename JFactoryPodioT<T>::CollectionT&& collection, std::string name) {
    /// InsertCollection inserts the provided PODIO collection into both the podio::Frame and then a JFactoryPodioT<T>

    auto frame = GetOrCreateFrame(shared_from_this());
    const auto& owned_collection = frame->put(std::move(collection), name);
    return InsertCollectionAlreadyInFrame<T>(&owned_collection, name);
}


template <typename T>
JFactoryPodioT<T>* JEvent::InsertCollectionAlreadyInFrame(const podio::CollectionBase* collection, std::string name) {
    /// InsertCollection inserts the provided PODIO collection into a JFactoryPodioT<T>. It assumes that the collection pointer
    /// is _already_ owned by the podio::Frame corresponding to this JEvent. This is meant to be used if you are starting out
    /// with a PODIO frame (e.g. a JEventSource that uses podio::ROOTReader).
    
    const auto* typed_collection = dynamic_cast<const typename T::collection_type*>(collection);
    if (typed_collection == nullptr) {
        throw JException("Attempted to insert a collection of the wrong type! name='%s', expected type='%s', actual type='%s'",
            name.c_str(), JTypeInfo::demangle<typename T::collection_type>().c_str(), collection->getDataTypeName().data());
    }

    // Users are allowed to Insert with tag="" if and only if that tag gets resolved by default tags.
    if (mUseDefaultTags && name.empty()) {
        auto defaultTag = mDefaultTags.find(JTypeInfo::demangle<T>());
        if (defaultTag != mDefaultTags.end()) name = defaultTag->second;
    }

    // Retrieve factory if it already exists, else create it
    JFactoryT<T>* factory = mFactorySet->GetFactory<T>(name);
    if (factory == nullptr) {
        factory = new JFactoryPodioT<T>();
        factory->SetTag(name);
        factory->SetLevel(GetLevel());
        mFactorySet->Add(factory);

        auto it = mPodioFactories.find(name);
        if (it != mPodioFactories.end()) {
            throw JException("InsertCollection failed because tag '%s' is not unique", name.c_str());
        }
        mPodioFactories[name] = factory;
    }

    // PODIO collections can only be inserted once, unlike regular JANA factories.
    if (factory->GetStatus() == JFactory::Status::Inserted  ||
        factory->GetStatus() == JFactory::Status::Processed) {

        throw JException("PODIO collections can only be inserted once, but factory with tag '%s' already has data", name.c_str());
    }

    // There's a chance that some user already added to the event's JFactorySet a
    // JFactoryT<PodioT> which ISN'T a JFactoryPodioT<T>. In this case, we cannot set the collection.
    JFactoryPodioT<T>* typed_factory = dynamic_cast<JFactoryPodioT<T>*>(factory);
    if (typed_factory == nullptr) {
        throw JException("Factory must inherit from JFactoryPodioT in order to use JEvent::GetCollection()");
    }

    typed_factory->SetCollectionAlreadyInFrame(typed_collection);
    typed_factory->SetInsertOrigin( mCallGraph.GetInsertDataOrigin() );
    return typed_factory;
}

#endif // JANA2_HAVE_PODIO



