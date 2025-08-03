
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JObject.h>
#include <JANA/JException.h>
#include <JANA/JFactoryT.h>
#include <JANA/JFactorySet.h>
#include <JANA/JLogger.h>
#include <JANA/JVersion.h>

#include <JANA/Components/JLightweightDatabundle.h>
#if JANA2_HAVE_PODIO
#include <JANA/Components/JPodioDatabundle.h>
#endif

#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Utils/JCallGraphRecorder.h>
#include <JANA/Utils/JCallGraphEntryMaker.h>
#include <JANA/Utils/JInspector.h>

#include <typeindex>
#include <vector>
#include <memory>
#include <atomic>


class JApplication;
class JEventSource;


class JEvent : public std::enable_shared_from_this<JEvent> {

private:
    JApplication* mApplication = nullptr;
    int32_t mRunNumber = 0;
    uint64_t mEventNumber = 0;
    mutable JFactorySet mFactorySet;
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


public:
    JEvent();
    explicit JEvent(JApplication* app);
    virtual ~JEvent();

    void SetRunNumber(int32_t aRunNumber){mRunNumber = aRunNumber;}
    void SetEventNumber(uint64_t aEventNumber){mEventNumber = aEventNumber;}
    void SetJApplication(JApplication* app){mApplication = app;}
    void SetJEventSource(JEventSource* aSource){mEventSource = aSource;}
    void SetDefaultTags(std::map<std::string, std::string> aDefaultTags){mDefaultTags=aDefaultTags; mUseDefaultTags = !mDefaultTags.empty();}
    void SetSequential(bool isSequential) {mIsBarrierEvent = isSequential;}

    JFactorySet* GetFactorySet() const { return &mFactorySet; }
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
    JEventLevel GetLevel() const { return mFactorySet.GetLevel(); }
    void SetLevel(JEventLevel level) { mFactorySet.SetLevel(level); }
    void SetEventIndex(int event_index) { mEventIndex = event_index; }
    int64_t GetEventIndex() const { return mEventIndex; }

    bool HasParent(JEventLevel level) const;
    const JEvent& GetParent(JEventLevel level) const;
    void SetParent(JEvent* parent);
    JEvent* ReleaseParent(JEventLevel level);
    int Release();

    // Lifecycle
    void Clear(bool processed_successfully=true);
    void Finish();

    JFactory* GetFactory(const std::string& object_name, const std::string& tag) const;
    std::vector<JFactory*> GetAllFactories() const;


    template<class T> JFactoryT<T>* GetFactory(const std::string& tag = "", bool throw_on_missing=false) const;
    template<class T> JLightweightDatabundleT<T>* GetLightweightDatabundle(const std::string& tag, bool throw_on_missing, bool call_factory_create) const;
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
    template <typename T> const typename T::collection_type* GetCollection(std::string name, bool throw_on_missing=true) const;
    template <typename T> void InsertCollection(typename T::collection_type&& collection, std::string name);
    template <typename T> void InsertCollectionAlreadyInFrame(const podio::CollectionBase* collection, std::string name);
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
    auto* databundle = mFactorySet.GetDatabundle(std::type_index(typeid(T)), resolved_tag);
    if (databundle == nullptr) {
        if (throw_on_missing) {
            JException ex("Could not find databundle with type_index=" + JTypeInfo::demangle<T>() + " and tag=" + tag);
            ex.show_stacktrace = false;
            mFactorySet.Print();
            throw ex;
        }
        return nullptr;
    };
    auto* factory = databundle->GetFactory();
    if (factory == nullptr) {
        if (throw_on_missing) {
            JException ex("No factory provided for databundle with type_index=" + JTypeInfo::demangle<T>() + " and tag=" + tag);
            ex.show_stacktrace = false;
            mFactorySet.Print();
            throw ex;
        }
        return nullptr;
    };
    auto* typed_factory = dynamic_cast<JFactoryT<T>*>(databundle->GetFactory());
    if (typed_factory == nullptr) {
        if (throw_on_missing) {
            JException ex("Factory does not inherit from JFactoryT<T> for databundle with type_index=" + JTypeInfo::demangle<T>() + " and tag=" + tag);
            ex.show_stacktrace = false;
            mFactorySet.Print();
            throw ex;
        }
        return nullptr;
    };
    return typed_factory;
}

template<class T>
JLightweightDatabundleT<T>* JEvent::GetLightweightDatabundle(const std::string& tag, bool throw_on_missing, bool call_factory_create) const {
    std::string resolved_tag = tag;
    if (mUseDefaultTags && tag.empty()) {
        auto defaultTag = mDefaultTags.find(JTypeInfo::demangle<T>());
        if (defaultTag != mDefaultTags.end()) resolved_tag = defaultTag->second;
    }
    auto* databundle = mFactorySet.GetDatabundle(std::type_index(typeid(T)), resolved_tag);
    if (databundle == nullptr) {
        if (throw_on_missing) {
            JException ex("Could not find databundle with type_index=" + JTypeInfo::demangle<T>() + " and tag=" + tag);
            ex.show_stacktrace = false;
            mFactorySet.Print();
            throw ex;
        }
        return nullptr;
    };
    auto* typed_databundle = dynamic_cast<JLightweightDatabundleT<T>*>(databundle);
    if (typed_databundle == nullptr) {
        if (throw_on_missing) {
            JException ex("Databundle with shortname '%s' does not inherit from JLightweightDatabundleT<%s>", tag.c_str(), JTypeInfo::demangle<T>().c_str());
            ex.show_stacktrace = false;
            mFactorySet.Print();
            throw ex;
        }
        return nullptr;
    }
    auto factory = databundle->GetFactory();
    if (call_factory_create && factory != nullptr) {
        // Always call JFactory::Create if we have it, because REGENERATE and GetObjects logic is extremely complex and might override databundle contents
        JCallGraphEntryMaker cg_entry(mCallGraph, factory); // times execution until this goes out of scope
        factory->Create(*this);
    }
    return typed_databundle;
}


/// GetFactoryAll returns all JFactoryT's for type T (each corresponds to a different tag).
/// This is useful when there are many different tags, or the tags are unknown, and the user
/// wishes to examine them all together.
template<class T>
inline std::vector<JFactoryT<T>*> JEvent::GetFactoryAll(bool throw_on_missing) const {
    std::vector<JFactoryT<T>*> factories;
    for (auto* factory : mFactorySet.GetAllFactories()) {
        auto typed_factory = dynamic_cast<JFactoryT<T>*>(factory);
        if (typed_factory != nullptr) {
            factories.push_back(typed_factory);
        }
    }
    if (factories.size() == 0) {
        if (throw_on_missing) {
            JException ex("Could not find any JFactoryT<" + JTypeInfo::demangle<T>() + "> (from any tag)");
            throw ex;
        }
    };
    return factories;
}


/// C-style getters

template<class T>
JFactoryT<T>* JEvent::GetSingle(const T* &t, const char *tag, bool exception_if_not_one) const {
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

    auto* databundle = GetLightweightDatabundle<T>(tag, true, true); // throw exception if databundle not found
    if (databundle->GetSize() != 1) {
        t = nullptr;
        if (exception_if_not_one) {
            throw JException("GetSingle<%s>: Databundle has wrong number of items: 1 expected, %d found.", JTypeInfo::demangle<T>().c_str(), databundle->GetSize());
        }
    }
    else {
        t = databundle->GetData().at(0);
    }
    return dynamic_cast<JFactoryT<T>*>(databundle->GetFactory());
}

/// Get conveniently returns one item from inside the JFactory. This should be used when the data in question
/// is optional and the caller wants to examine the result and decide how to proceed. The caller should embed this
/// inside an if-block. Get updates the `destination` out parameter and returns a pointer to the enclosing JFactory.
/// - If the factory is missing, GetSingle throws an exception.
/// - If the factory exists but contains no items, GetSingle updates the `destination` to point to nullptr.
/// - If the factory contains exactly one item, GetSingle updates the `destination` to point to that item.
/// - If the factory contains more than one item, GetSingle updates the `destination` to point to the first time.
template<class T>
JFactoryT<T>* JEvent::Get(const T** destination, const std::string& tag) const {
    auto* databundle = GetLightweightDatabundle<T>(tag, true, true); // throw exception if databundle not found
    if (databundle->GetSize() == 0) {
        *destination = nullptr;
    }
    else {
        *destination = databundle->GetData().at(0);
    }
    return dynamic_cast<JFactoryT<T>*>(databundle->GetFactory());
}


template<class T>
JFactoryT<T>* JEvent::Get(std::vector<const T*>& destination, const std::string& tag, bool strict) const
{
    auto* databundle = GetLightweightDatabundle<T>(tag, strict, true); // throw exception if databundle not found
    if (databundle != nullptr) {
        for (auto* item : databundle->GetData()) {
            destination.push_back(item);
        }
    }
    return dynamic_cast<JFactoryT<T>*>(databundle->GetFactory());
}


/// GetAll returns all JObjects of (child) type T, regardless of tag.
template<class T>
void JEvent::GetAll(std::vector<const T*>& destination) const {
    auto factories = GetFactoryAll<T>(true);
    for (auto factory : factories) {
        auto iterators = factory->CreateAndGetData(*this);
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

    auto databundle = GetLightweightDatabundle<T>(tag, true, true); // Excepts rather than returns nullptr
    if (databundle->GetSize() == 0) {
        return nullptr;
    }
    return databundle->GetData().at(0);
}



/// GetSingleStrict conveniently returns one item from inside the JFactory. This should be used when the data in
/// question is mandatory, and its absence indicates an error which should stop execution. The caller does not need
/// to embed this in an if- or try-catch block; it can be a one-liner.
/// - If the factory is missing, GetSingleStrict throws an exception
/// - If the factory exists but contains no items, GetSingleStrict throws an exception
/// - If the factory contains more than one item, GetSingleStrict throws an exception
template<class T> const T* JEvent::GetSingleStrict(const std::string& tag) const {

    auto databundle = GetLightweightDatabundle<T>(tag, true, true); // Excepts rather than returns nullptr
    if (databundle->GetSize() == 0) {
        JException ex("GetSingle failed due to missing %d", NAME_OF(T));
        ex.show_stacktrace = false;
        throw ex;
        return nullptr;
    }
    else if (databundle->GetSize() > 1) {
        JException ex("GetSingle failed due to too many %d", NAME_OF(T));
        ex.show_stacktrace = false;
        throw ex;
    }
    return databundle->GetData().at(0);
}

template<class T>
std::vector<const T*> JEvent::Get(const std::string& tag, bool strict) const {

    auto databundle = GetLightweightDatabundle<T>(tag, strict, true);
    std::vector<const T*> vec;
    if (databundle != nullptr) { // databundle might be nullptr if strict=false
        for (auto x: databundle->GetData()) {
            vec.push_back(x);
        }
    }
    return vec;
}

template<class T>
typename JFactoryT<T>::PairType JEvent::GetIterators(const std::string& tag) const {

    auto databundle = GetLightweightDatabundle<T>(tag, true, true);
    auto& data = databundle->GetData();
    return std::make_pair(data.cbegin(), data.cend());
}

/// GetAll returns all JObjects of (child) type T, regardless of tag.
template<class T>
std::vector<const T*> JEvent::GetAll() const {

    std::vector<const T*> results;

    for (auto databundle : mFactorySet.GetDatabundles(std::type_index(typeid(T)))) {
        auto fac = databundle->GetFactory();
        if (fac != nullptr) {
            fac->Create(*this);
        }
        auto typed_databundle = dynamic_cast<JLightweightDatabundleT<T>*>(databundle);
        if (typed_databundle != nullptr) {
            for (auto* item : typed_databundle->GetData()) {
                results.push_back(item);
            }
        }
    }
    return results;
}

// GetAllChildren will furnish a map { (type_name,tag_name) : [BaseClass*] } containing all JFactoryT<T> data where
// T inherits from BaseClass. Note that this _won't_ compute any results (unlike GetAll) because this is meant for
// things like visualizing and persisting DSTs.
// TODO: This is conceptually inconsistent with GetAll. Reconcile.

template<class S>
std::map<std::pair<std::string, std::string>, std::vector<S*>> JEvent::GetAllChildren() const {
    std::map<std::pair<std::string, std::string>, std::vector<S*>> results;
    for (JFactory* factory : mFactorySet.GetAllFactories()) {
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

    auto* databundle = GetLightweightDatabundle<T>(tag, false, false);

    if (databundle == nullptr) {
        auto* factory = new JFactoryT<T>;
        factory->SetTag(tag);
        factory->SetLevel(mFactorySet.GetLevel());
        mFactorySet.Add(factory);
        databundle = GetLightweightDatabundle<T>(tag, false, false);
    }

    databundle->SetStatus(JDatabundle::Status::Inserted);
    databundle->GetData().push_back(item);

    auto* factory = databundle->GetFactory();
    if (factory != nullptr) {
        factory->SetStatus(JFactory::Status::Inserted); // for when items is empty
        factory->SetCreationStatus(JFactory::CreationStatus::Inserted); // for when items is empty
        factory->SetInsertOrigin( mCallGraph.GetInsertDataOrigin() ); // (see note at top of JCallGraphRecorder.h)
        return dynamic_cast<JFactoryT<T>*>(factory);
    }
    throw JException("Attempted to call JEvent::Insert without an underlying JFactoryT. Hint: Did you previously use Output<T>?");
    // TODO: Have Insert() return the databundle instead of the factory
}

template <class T>
inline JFactoryT<T>* JEvent::Insert(const std::vector<T*>& items, const std::string& tag) const {

    auto* databundle = GetLightweightDatabundle<T>(tag, false, false);

    if (databundle == nullptr) {
        auto* factory = new JFactoryT<T>;
        factory->SetTag(tag);
        factory->SetLevel(mFactorySet.GetLevel());
        mFactorySet.Add(factory);
        databundle = GetLightweightDatabundle<T>(tag, false, false);
    }

    databundle->SetStatus(JDatabundle::Status::Inserted);
    databundle->GetData() = items;

    auto* factory = databundle->GetFactory();
    if (factory != nullptr) {
        factory->SetStatus(JFactory::Status::Inserted); // for when items is empty
        factory->SetCreationStatus(JFactory::CreationStatus::Inserted); // for when items is empty
        factory->SetInsertOrigin( mCallGraph.GetInsertDataOrigin() ); // (see note at top of JCallGraphRecorder.h)
        return dynamic_cast<JFactoryT<T>*>(factory);
    }
    throw JException("Attempted to call JEvent::Insert without an underlying JFactoryT. Hint: Did you previously use Output<T>?");
    // TODO: Have Insert() return the databundle instead of the factory
}



#if JANA2_HAVE_PODIO

inline std::vector<std::string> JEvent::GetAllCollectionNames() const {
    std::vector<std::string> unique_names;
    for (auto databundle : mFactorySet.GetAllDatabundles()) {
        if (dynamic_cast<JPodioDatabundle*>(databundle) != nullptr) {
            unique_names.push_back(databundle->GetUniqueName());
        }
    }
    return unique_names;
}

inline const podio::CollectionBase* JEvent::GetCollectionBase(std::string unique_name, bool throw_on_missing) const {
    auto* bundle = mFactorySet.GetDatabundle(unique_name);
    if (bundle == nullptr) {
        if (throw_on_missing) {
            throw JException("Missing databundle with uniquename '%s'", unique_name.c_str());
        }
        return nullptr;
    }

    auto* typed_bundle = dynamic_cast<JPodioDatabundle*>(bundle);
    if (typed_bundle == nullptr) {
        if (throw_on_missing) {
            throw JException("Databundle with uniquename '%s' is not a JPodioDatabundle", unique_name.c_str());
        }
        return nullptr;
    }

    if (typed_bundle->GetStatus() == JDatabundle::Status::Empty) {
        auto* fac = typed_bundle->GetFactory();
        if (fac != nullptr) {
            JCallGraphEntryMaker cg_entry(mCallGraph, fac); // times execution until this goes out of scope
            fac->Create(*this);
        }
    }

    return typed_bundle->GetCollection();
}


template <typename T>
const typename T::collection_type* JEvent::GetCollection(std::string name, bool throw_on_missing) const {

    auto collection = GetCollectionBase(name, throw_on_missing);
    auto* typed_collection = dynamic_cast<const typename T::collection_type*>(collection);
    if (throw_on_missing && typed_collection == nullptr) {
        throw JException("Databundle with uniquename '%s' does not contain %s", JTypeInfo::demangle<typename T::collection_type>().c_str());
    }
    return typed_collection;
}


template <typename T>
void JEvent::InsertCollection(typename T::collection_type&& collection, std::string name) {
    /// InsertCollection inserts the provided PODIO collection into both the podio::Frame and then a JPodioDatabundle

    podio::Frame* frame = nullptr;
    auto* bundle = mFactorySet.GetDatabundle("podio::Frame");

    if (bundle == nullptr) {
        LOG << "No frame databundle found, inserting new dummy JFactoryT.";
        frame = new podio::Frame();
        Insert(frame, ""); 
        // Eventually we'll insert a databundle directly without the dummy JFactoryT
        // However, we obviously need JEvent::GetSingle<podio::Frame>() to work, which 
        // requires re-plumbing all or most of the Get*() methods to no longer rely on JFactoryT.
    }
    else {
        JLightweightDatabundleT<podio::Frame>* typed_bundle = nullptr;
        typed_bundle = dynamic_cast<JLightweightDatabundleT<podio::Frame>*>(bundle);
        if (typed_bundle == nullptr) {
            throw JException("Databundle with unique_name 'podio::Frame' is not a JLightweightDatabundleT");
        }
        if (typed_bundle->GetSize() == 0) {
            LOG << "Found typed bundle with no frame. Creating new frame.";
            typed_bundle->GetData().push_back(new podio::Frame);
            typed_bundle->SetStatus(JDatabundle::Status::Inserted);
        }
        frame = typed_bundle->GetData().at(0);
    }

    const auto& owned_collection = frame->put(std::move(collection), name);
    InsertCollectionAlreadyInFrame<T>(&owned_collection, name);
}


template <typename T>
void JEvent::InsertCollectionAlreadyInFrame(const podio::CollectionBase* collection, std::string unique_name) {
    /// InsertCollection inserts the provided PODIO collection into a JPodioDatabundle. It assumes that the collection pointer
    /// is _already_ owned by the podio::Frame corresponding to this JEvent. This is meant to be used if you are starting out
    /// with a PODIO frame (e.g. a JEventSource that uses podio::ROOTReader).

    const auto* typed_collection = dynamic_cast<const typename T::collection_type*>(collection);
    if (typed_collection == nullptr) {
        mFactorySet.Print();
        throw JException("Attempted to insert a collection of the wrong type! name='%s', expected type='%s', actual type='%s'",
            unique_name.c_str(), JTypeInfo::demangle<typename T::collection_type>().c_str(), collection->getDataTypeName().data());
    }

    // Users are allowed to Insert with tag="" if and only if that tag gets resolved by default tags.
    if (mUseDefaultTags && unique_name.empty()) {
        auto defaultTag = mDefaultTags.find(JTypeInfo::demangle<T>());
        if (defaultTag != mDefaultTags.end()) unique_name = defaultTag->second;
    }

    // Retrieve factory if it already exists, else create it

    JDatabundle* bundle = mFactorySet.GetDatabundle(unique_name);
    JPodioDatabundle* typed_bundle = nullptr;

    if (bundle == nullptr) {
        typed_bundle = new JPodioDatabundle();
        typed_bundle->SetUniqueName(unique_name);
        typed_bundle->SetTypeIndex(std::type_index(typeid(T)));
        typed_bundle->SetTypeName(JTypeInfo::demangle<T>());
        mFactorySet.Add(typed_bundle); 
        // Note that this transfers ownership to the JFactorySet because there's no corresponding JFactory
    }
    else {
        typed_bundle = dynamic_cast<JPodioDatabundle*>(bundle);
        if (typed_bundle == nullptr) {
            mFactorySet.Print();
            throw JException("Databundle with unique_name='%s' must be a JPodioDatabundle in order to insert a Podio collection", unique_name.c_str());
        }
        if (typed_bundle->GetStatus() != JDatabundle::Status::Empty) {
            // PODIO collections can only be inserted once, unlike regular JANA factories.
            mFactorySet.Print();
            throw JException("A Podio collection with unique_name='%s' is already present and cannot be overwritten", unique_name.c_str());
        }
    }

    typed_bundle->SetStatus(JDatabundle::Status::Inserted);
    typed_bundle->SetCollection(typed_collection);
    auto fac = typed_bundle->GetFactory();
    if (fac) {
        fac->SetStatus(JFactory::Status::Inserted);
    }
}

#endif // JANA2_HAVE_PODIO



