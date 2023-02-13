
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JFactory_h_
#define _JFactory_h_

#include <JANA/JException.h>
#include <JANA/Utils/JAny.h>
#include <JANA/Utils/JCallGraphRecorder.h>

#include <string>
#include <typeindex>
#include <memory>
#include <limits>
#include <atomic>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <functional>


namespace podio {
    class Frame;
}
class JEvent;
class JObject;
class JApplication;

class JFactory {
public:

    enum class Status {Uninitialized, Unprocessed, Processed, Inserted};
    enum class CreationStatus { NotCreatedYet, Created, Inserted, InsertedViaGetObjects, NeverCreated };

    enum JFactory_Flags_t {
        JFACTORY_NULL = 0x00,
        PERSISTENT = 0x01,
        WRITE_TO_OUTPUT = 0x02,
        NOT_OBJECT_OWNER = 0x04
    };

    JFactory(std::string aName, std::string aTag = "")
    : mObjectName(std::move(aName)), mTag(std::move(aTag)), mStatus(Status::Uninitialized) {};

    virtual ~JFactory() = default;


    std::string GetName() const __attribute__ ((deprecated))  { return mObjectName; }

    std::string GetTag() const { return mTag; }
    std::string GetObjectName() const { return mObjectName; }
    std::string GetFactoryName() const { return mFactoryName; }
    std::string GetPluginName() const { return mPluginName; }
    Status GetStatus() const { return mStatus; }
    CreationStatus GetCreationStatus() const { return mCreationStatus; }
    JCallGraphRecorder::JDataOrigin GetInsertOrigin() const { return m_insert_origin; } ///< If objects were placed here by JEvent::Insert() this records whether that call was made from a source or factory.

    uint32_t GetPreviousRunNumber(void) const { return mPreviousRunNumber; }


    void SetName(std::string objectName) __attribute__ ((deprecated)) { mObjectName = std::move(objectName); }

    void SetTag(std::string tag) { mTag = std::move(tag); }
    void SetObjectName(std::string objectName) { mObjectName = std::move(objectName); }
    void SetFactoryName(std::string factoryName) { mFactoryName = std::move(factoryName); }
    void SetPluginName(std::string pluginName) { mPluginName = std::move(pluginName); }
    void SetStatus(Status status){ mStatus = status; }
    void SetCreationStatus(CreationStatus status){ mCreationStatus = status; }
    void SetInsertOrigin(JCallGraphRecorder::JDataOrigin origin) { m_insert_origin = origin; } ///< Called automatically by JEvent::Insert() to records whether that call was made by a source or factory.

    void SetPreviousRunNumber(uint32_t aRunNumber) { mPreviousRunNumber = aRunNumber; }

    /// Get all flags in the form of a single word
    inline uint32_t GetFactoryFlags() const { return mFlags; }

    /// Set a flag (or flags)
    inline void SetFactoryFlag(JFactory_Flags_t f) {
        mFlags |= (uint32_t) f;
    }

    /// Clear a flag (or flags)
    inline void ClearFactoryFlag(JFactory_Flags_t f) {
        mFlags &= ~(uint32_t) f;
    }

    /// Test if a flag (or set of flags) is set
    inline bool TestFactoryFlag(JFactory_Flags_t f) const {
        return (mFlags & (uint32_t) f) == (uint32_t) f;
    }

    /// Get data source value depending on how objects came to be here. (Used mainly by JEvent::Get() )
    inline JCallGraphRecorder::JDataSource GetDataSource() const {
        JCallGraphRecorder::JDataSource datasource = JCallGraphRecorder::DATA_FROM_FACTORY;
         if( mCreationStatus == JFactory::CreationStatus::Inserted ){
            if( m_insert_origin == JCallGraphRecorder::ORIGIN_FROM_SOURCE ){
                datasource = JCallGraphRecorder::DATA_FROM_SOURCE;
            }else{
                datasource = JCallGraphRecorder::DATA_FROM_CACHE; // Really came from factory, but if Inserted, it was a secondary data type.
            }
        }
        return datasource;
    }

    // Overloaded by JFactoryT
    virtual std::type_index GetObjectType() const = 0;

    virtual void ClearData() = 0;


    // Overloaded by user Factories
    virtual void Init() {}
    virtual void BeginRun(const std::shared_ptr<const JEvent>&) {}
    virtual void ChangeRun(const std::shared_ptr<const JEvent>&) {}
    virtual void EndRun() {}
    virtual void Process(const std::shared_ptr<const JEvent>&) {}
    virtual void Finish() {}

    virtual std::size_t GetNumObjects() const {
        return 0;
    }


    /// Access the encapsulated data, performing an upcast if necessary. This is useful for extracting data from
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

    /// Create() calls JFactory::Init,BeginRun,Process in an invariant-preserving way without knowing the exact
    /// type of object contained. It returns the number of objects created. In order to access said objects,
    /// use JFactory::GetAs().
    void Create(const std::shared_ptr<const JEvent>& event);

    /// JApplication setter. This is meant to be used under the hood.
    void SetApplication(JApplication* app) { mApp = app; }

    /// JApplication getter. This is meant to be called by user-defined JFactories which need to
    /// acquire parameter values or services from JFactory::Init()
    JApplication* GetApplication() { return mApp; }


    virtual void Set(const std::vector<JObject *> &data) = 0;
    virtual void Insert(JObject *data) = 0;

protected:

    std::string mPluginName;
    std::string mFactoryName;
    std::string mObjectName;
    std::string mTag;
    uint32_t mFlags = 0;
    int32_t mPreviousRunNumber = -1;
    JApplication* mApp = nullptr;
    std::unordered_map<std::type_index, std::unique_ptr<JAny>> mUpcastVTable;
    podio::Frame* mFrame;

    mutable Status mStatus = Status::Uninitialized;
    mutable JCallGraphRecorder::JDataOrigin m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE; // (see note at top of JCallGraphRecorder.h)

    CreationStatus mCreationStatus = CreationStatus::NotCreatedYet;
    mutable std::mutex mMutex;

    // Used to make sure Init is called only once
    std::once_flag mInitFlag;
};

// Because C++ doesn't support templated virtual functions, we implement our own dispatch table, mUpcastVTable.
// This means that the JFactoryT is forced to manually populate this table by calling JFactoryT<T>::EnableGetAs.
// We have the option to make the vtable be a static member of JFactoryT<T>, but we have chosen not to because:
//
//   1. It would be inconsistent with the fact that the user is supposed to call EnableGetAs in the ctor
//   2. People in the future may want to generalize GetAs to support user-defined S* -> T* conversions (which I don't recommend)
//   3. The size of the vtable is expected to be very small (<10 elements, most likely 2)

template<typename S>
std::vector<S*> JFactory::GetAs() {
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

#endif // _JFactory_h_

