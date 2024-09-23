
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JObject.h>
#include <JANA/JException.h>
#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>
#include <JANA/JLogger.h>
#include <JANA/JVersion.h>
#include <JANA/Components/JDataBundle.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Utils/JCallGraphRecorder.h>
#include <JANA/Utils/JCallGraphEntryMaker.h>
#include <JANA/Utils/JInspector.h>

#include <typeindex>
#include <vector>
#include <cstddef>
#include <memory>
#include <atomic>

#if JANA2_HAVE_PODIO
#include <JANA/Components/JPodioDataBundle.h>
#include <podio/Frame.h>
#endif

class JApplication;
class JEventSource;


class JEvent : public std::enable_shared_from_this<JEvent>
{
    public:

        explicit JEvent(JApplication* aApplication=nullptr) : mInspector(&(*this)) {
            mApplication = aApplication;
            mFactorySet = new JFactorySet();
        }
        virtual ~JEvent() {
            if (mFactorySet != nullptr) mFactorySet->Release();
            delete mFactorySet;
        }

        void SetFactorySet(JFactorySet* aFactorySet) {
            delete mFactorySet;
            mFactorySet = aFactorySet;
        }

        JFactorySet* GetFactorySet() const { return mFactorySet; }

        JFactory* GetFactory(const std::string& object_name, const std::string& tag) const;
        std::vector<JFactory*> GetAllFactories() const;
        template<class T> JFactoryT<T>* GetFactory(const std::string& tag = "", bool throw_on_missing=false) const;
        template<class T> std::vector<JFactoryT<T>*> GetFactoryAll(bool throw_on_missing = false) const;

        template<class T> JMetadata<T> GetMetadata(const std::string& tag = "") const;

        //OBJECTS
        // C style getters
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

        // JANA1 compatibility getters
        template<class T> JFactoryT<T>* GetSingle(const T* &t, const char *tag="", bool exception_if_not_one=true) const;

        // Insert
        template <class T> JFactoryT<T>* Insert(T* item, const std::string& aTag = "") const;
        template <class T> JFactoryT<T>* Insert(const std::vector<T*>& items, const std::string& tag = "") const;

        // PODIO
#if JANA2_HAVE_PODIO
        std::vector<std::string> GetAllCollectionNames() const;
        const podio::CollectionBase* GetCollectionBase(std::string name, bool throw_on_missing=true) const;
        template <typename PodioT> const typename PodioT::collection_type* GetCollection(std::string name, bool throw_on_missing=true) const;
        template <typename PodioT> JPodioDataBundle* InsertCollection(typename PodioT::collection_type&& collection, std::string name);
        template <typename PodioT> JPodioDataBundle* InsertCollectionAlreadyInFrame(const podio::CollectionBase* collection, std::string name);
#endif

        // EXPERIMENTAL NEW THING
        JDataBundle* GetDataBundle(const std::string& name, bool create) const;

        //SETTERS
        void SetRunNumber(int32_t aRunNumber){mRunNumber = aRunNumber;}
        void SetEventNumber(uint64_t aEventNumber){mEventNumber = aEventNumber;}
        void SetJApplication(JApplication* app){mApplication = app;}
        void SetJEventSource(JEventSource* aSource){mEventSource = aSource;}
        void SetDefaultTags(std::map<std::string, std::string> aDefaultTags){mDefaultTags=aDefaultTags; mUseDefaultTags = !mDefaultTags.empty();}
        void SetSequential(bool isSequential) {mIsBarrierEvent = isSequential;}

        //GETTERS
        int32_t GetRunNumber() const {return mRunNumber;}
        uint64_t GetEventNumber() const {return mEventNumber;}
        JApplication* GetJApplication() const {return mApplication;}
        JEventSource* GetJEventSource() const {return mEventSource; }
        JCallGraphRecorder* GetJCallGraphRecorder() const {return &mCallGraph;}
        JInspector* GetJInspector() const {return &mInspector;}
        void Inspect() const { mInspector.Loop();} // TODO: Force this not to be inlined AND used so it is defined in libJANA.a
        bool GetSequential() const {return mIsBarrierEvent;}
        friend class JEventPool;


        // Hierarchical
        JEventLevel GetLevel() const { return mFactorySet->GetLevel(); }
        void SetLevel(JEventLevel level) { mFactorySet->SetLevel(level); }
        void SetEventIndex(int event_index) { mEventIndex = event_index; }
        int64_t GetEventIndex() const { return mEventIndex; }

        bool HasParent(JEventLevel level) const {
            for (const auto& pair : mParents) {
                if (pair.first == level) return true;
            }
            return false;
        }

        const JEvent& GetParent(JEventLevel level) const {
            for (const auto& pair : mParents) {
                if (pair.first == level) return *(*(pair.second));
            }
            throw JException("Unable to find parent at level %s", 
                             toString(level).c_str());
        }

        void SetParent(std::shared_ptr<JEvent>* parent) {
            JEventLevel level = parent->get()->GetLevel();
            for (const auto& pair : mParents) {
                if (pair.first == level) throw JException("Event already has a parent at level %s", 
                                                          toString(parent->get()->GetLevel()).c_str());
            }
            mParents.push_back({level, parent});
            parent->get()->mReferenceCount.fetch_add(1);
        }

        std::shared_ptr<JEvent>* ReleaseParent(JEventLevel level) {
            if (mParents.size() == 0) {
                throw JException("ReleaseParent failed: child has no parents!");
            }
            auto pair = mParents.back();
            if (pair.first != level) {
                throw JException("JEvent::ReleaseParent called out of level order: Caller expected %s, but parent was actually %s", 
                        toString(level).c_str(), toString(pair.first).c_str());
            }
            mParents.pop_back();
            auto remaining_refs = pair.second->get()->mReferenceCount.fetch_sub(1);
            if (remaining_refs < 1) { // Remember, this was fetched _before_ the last subtraction
                throw JException("Parent refcount has gone negative!");
            }
            if (remaining_refs == 1) {
                return pair.second; 
                // Parent is no longer shared. Transfer back to arrow
            }
            else {
                return nullptr; // Parent is still shared by other children
            }
        }

        void Release() {
            auto remaining_refs = mReferenceCount.fetch_sub(1);
            if (remaining_refs < 0) {
                throw JException("JEvent's own refcount has gone negative!");
            }
        }

        void Reset() {
            mReferenceCount = 1;
        }


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

        // Hierarchical stuff
        std::vector<std::pair<JEventLevel, std::shared_ptr<JEvent>*>> mParents;
        std::atomic_int mReferenceCount {1};
        int64_t mEventIndex = -1;


};

/// Insert() allows an EventSource to insert items directly into the JEvent,
/// removing the need for user-extended JEvents and/or JEventSource::GetObjects(...)
/// Repeated calls to Insert() will append to the previous data rather than overwrite it,
/// which saves the user from having to allocate a throwaway vector and requires less error handling.
template <class T>
inline JFactoryT<T>* JEvent::Insert(T* item, const std::string& tag) const {

    std::string object_name = JTypeInfo::demangle<T>();

    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(object_name);
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    auto untyped_factory = mFactorySet->GetFactory(std::type_index(typeid(T)), object_name, resolved_tag);
    JFactoryT<T>* typed_factory;
    if (untyped_factory == nullptr) {
        typed_factory = new JFactoryT<T>;
        typed_factory->SetTag(tag);
        typed_factory->SetLevel(mFactorySet->GetLevel());
        mFactorySet->Add(typed_factory);
    }
    else {
        typed_factory = dynamic_cast<JFactoryT<T>*>(untyped_factory);
        if (typed_factory == nullptr) {
            throw JException("Retrieved factory is not a JFactoryT!");
        }
    }
    typed_factory->Insert(item);
    typed_factory->SetInsertOrigin( mCallGraph.GetInsertDataOrigin() ); // (see note at top of JCallGraphRecorder.h)
    return typed_factory;
}

template <class T>
inline JFactoryT<T>* JEvent::Insert(const std::vector<T*>& items, const std::string& tag) const {

    std::string object_name = JTypeInfo::demangle<T>();
    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(object_name);
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    auto untyped_factory = mFactorySet->GetFactory(std::type_index(typeid(T)), object_name, resolved_tag);
    JFactoryT<T>* typed_factory;
    if (untyped_factory == nullptr) {
        typed_factory = new JFactoryT<T>;
        typed_factory->SetTag(tag);
        typed_factory->SetLevel(mFactorySet->GetLevel());
        mFactorySet->Add(typed_factory);
    }
    else {
        typed_factory = dynamic_cast<JFactoryT<T>*>(untyped_factory);
        if (typed_factory == nullptr) {
            throw JException("Retrieved factory is not a JFactoryT!");
        }
    }
    for (T* item : items) {
        typed_factory->Insert(item);
    }
    typed_factory->SetStatus(JFactory::Status::Inserted); // for when items is empty
    typed_factory->SetCreationStatus(JFactory::CreationStatus::Inserted); // for when items is empty
    typed_factory->SetInsertOrigin( mCallGraph.GetInsertDataOrigin() ); // (see note at top of JCallGraphRecorder.h)
    return typed_factory;
}

/// GetFactory() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory
inline JFactory* JEvent::GetFactory(const std::string& object_name, const std::string& tag) const {
    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(object_name);
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    return mFactorySet->GetFactory(object_name, resolved_tag);
}

/// GetAllFactories() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory
inline std::vector<JFactory*> JEvent::GetAllFactories() const {
    return mFactorySet->GetAllFactories();
}

/// GetFactory() should be used with extreme care because it subverts the JEvent abstraction.
/// Most historical uses of GetFactory are far better served by JMultifactory.
template<class T>
inline JFactoryT<T>* JEvent::GetFactory(const std::string& tag, bool throw_on_missing) const
{
    std::string object_name = JTypeInfo::demangle<T>();
    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(object_name);
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    auto factory = mFactorySet->GetFactory(std::type_index(typeid(T)), object_name, resolved_tag);
    if (factory == nullptr) {
        if (throw_on_missing) {
            JException ex("Could not find JFactory producing '%s' with tag '%s'", object_name.c_str(), resolved_tag.c_str());
            ex.show_stacktrace = false;
            throw ex;
        }
        return nullptr;
    };
    auto typed_factory = dynamic_cast<JFactoryT<T>*>(factory);
    if (typed_factory == nullptr) {
        JException ex("JFactory producing '%s' with tag '%s' is not a JFactoryT!", object_name.c_str(), resolved_tag.c_str());
        throw ex;
    };
    return typed_factory;
}


/// GetMetadata() provides access to any metadata generated by the underlying JFactory during Process()
template<class T>
[[deprecated("Use JMultifactory instead")]]
inline JMetadata<T> JEvent::GetMetadata(const std::string& tag) const {

    auto factory = GetFactory<T>(tag, true);
    // Make sure that JFactoryT::Process has already been called before returning the metadata
    factory->CreateAndGetData(this->shared_from_this());
    return factory->GetMetadata();
}


/// C-style getters

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

/// GetFactoryAll returns all JFactoryT's for type T (each corresponds to a different tag).
/// This is useful when there are many different tags, or the tags are unknown, and the user
/// wishes to examine them all together.
template<class T>
inline std::vector<JFactoryT<T>*> JEvent::GetFactoryAll(bool throw_on_missing) const {
    std::vector<JFactoryT<T>*> results;
    std::string object_name = JTypeInfo::demangle<T>();
    auto factories = mFactorySet->GetAllFactories(std::type_index(typeid(T)), object_name);
    if (factories.size() == 0) {
        if (throw_on_missing) {
            JException ex("Could not find any JFactoryT<%s> (from any tag)", object_name.c_str());
            ex.show_stacktrace = false;
            throw ex;
        }
    };
    for (auto* fac : factories) {
        auto fac_typed = dynamic_cast<JFactoryT<T>*>(fac);
        if (fac_typed != nullptr) {
            results.push_back(fac_typed);
        }
    }
    return results;
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


template<class T>
typename JFactoryT<T>::PairType JEvent::GetIterators(const std::string& tag) const {
    auto factory = GetFactory<T>(tag, true);
    JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
    auto iters = factory->CreateAndGetData(this->shared_from_this());
    return iters;
}


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

inline JDataBundle* JEvent::GetDataBundle(const std::string& name, bool create) const {

    auto* storage = mFactorySet->GetDataBundle(name);
    if (storage == nullptr) return nullptr;
    auto fac = storage->GetFactory();

    if (fac != nullptr && create) {

        // The regenerate logic lives out here now
        if ((storage->GetStatus() == JDataBundle::Status::Empty) || 
            (fac->TestFactoryFlag(JFactory::JFactory_Flags_t::REGENERATE))) {

            // If this was inserted, there would be no factory to run
            // fac->Create() will short-circuit if something was already inserted
            JCallGraphEntryMaker cg_entry(mCallGraph, fac); // times execution until this goes out of scope
            fac->Create(this->shared_from_this());
        }
    }
    return storage;
}

#if JANA2_HAVE_PODIO

inline std::vector<std::string> JEvent::GetAllCollectionNames() const {
    return mFactorySet->GetAllDataBundleNames();
}

inline const podio::CollectionBase* JEvent::GetCollectionBase(std::string name, bool throw_on_missing) const {
    auto* storage = GetDataBundle(name, true);
    if (storage != nullptr) {
        auto* podio_storage = dynamic_cast<JPodioDataBundle*>(storage);
        if (podio_storage == nullptr) {
            throw JException("Not a podio collection: %s", name.c_str());
        }
        else {
            return podio_storage->GetCollection();
        }
    }
    else if (throw_on_missing) {
        throw JException("Collection not found: '%s'", name.c_str());
    }
    return nullptr;
}

template <typename T>
const typename T::collection_type* JEvent::GetCollection(std::string name, bool throw_on_missing) const {
    auto* coll = GetDataBundle(name, true);
    if (coll != nullptr) {
        auto* podio_coll = dynamic_cast<JPodioDataBundle*>(coll);
        if (podio_coll == nullptr) {
            throw JException("Not a podio collection: %s", name.c_str());
        }
        else {
            auto coll = podio_coll->GetCollection();
            auto typed_coll = dynamic_cast<const typename T::collection_type*>(coll);
            if (typed_coll == nullptr) {
                throw JException("Unable to cast Podio collection to %s", JTypeInfo::demangle<typename T::collection_type>().c_str());
            }
            return typed_coll;
        }
    }
    else if (throw_on_missing) {
        throw JException("Collection not found: '%s'", name.c_str());
    }
    return nullptr;
}


template <typename PodioT>
JPodioDataBundle* JEvent::InsertCollection(typename PodioT::collection_type&& collection, std::string name) {
    /// InsertCollection inserts the provided PODIO collection into both the podio::Frame and then a JFactoryPodioT<T>

    podio::Frame* frame = nullptr;
    try {
        frame = const_cast<podio::Frame*>(GetSingle<podio::Frame>(""));
        if (frame == nullptr) {
            frame = new podio::Frame;
            Insert(frame);
        }
    }
    catch (...) {
        frame = new podio::Frame;
        Insert(frame);
    }
    const auto& owned_collection = frame->put(std::move(collection), name);
    return InsertCollectionAlreadyInFrame<PodioT>(&owned_collection, name);
}


template <typename PodioT>
JPodioDataBundle* JEvent::InsertCollectionAlreadyInFrame(const podio::CollectionBase* collection, std::string name) {
    /// InsertCollection inserts the provided PODIO collection into a JPodioStorage. It assumes that the collection pointer
    /// is _already_ owned by the podio::Frame corresponding to this JEvent. This is meant to be used if you are starting out
    /// with a PODIO frame (e.g. a JEventSource that uses podio::ROOTFrameReader).
    
    const auto* typed_collection = dynamic_cast<const typename PodioT::collection_type*>(collection);
    if (typed_collection == nullptr) {
        throw JException("Attempted to insert a collection of the wrong type! name='%s', expected type='%s', actual type='%s'",
            name.c_str(), JTypeInfo::demangle<typename PodioT::collection_type>().c_str(), collection->getDataTypeName().data());
    }

    // Users are allowed to Insert with tag="" if and only if that tag gets resolved by default tags.
    if (mUseDefaultTags && name.empty()) {
        auto defaultTag = mDefaultTags.find(JTypeInfo::demangle<PodioT>());
        if (defaultTag != mDefaultTags.end()) name = defaultTag->second;
    }

    // Retrieve storage if it already exists, else create it
    auto storage = mFactorySet->GetDataBundle(name);

    if (storage == nullptr) {
        // No factories already registered this! E.g. from an event source
        auto coll = new JPodioDataBundle;
        coll->SetUniqueName(name);
        coll->SetTypeName(JTypeInfo::demangle<PodioT>());
        coll->SetStatus(JDataBundle::Status::Inserted);
        coll->SetInsertOrigin(mCallGraph.GetInsertDataOrigin());
        coll->SetCollection(typed_collection);
        mFactorySet->Add(coll);
        return coll;
    }
    else {
        // This is overriding a factory
        // Check that we only inserted this collection once
        if (storage->GetStatus() != JDataBundle::Status::Empty) {
            throw JException("Collections can only be inserted once!");
        }
        auto typed_storage = dynamic_cast<JPodioDataBundle*>(storage);
        typed_storage->SetCollection(typed_collection);
        typed_storage->SetStatus(JDataBundle::Status::Inserted);
        typed_storage->SetInsertOrigin(mCallGraph.GetInsertDataOrigin());
        return typed_storage;
    }
}

#endif // JANA2_HAVE_PODIO



