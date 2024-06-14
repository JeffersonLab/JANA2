
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once 

#include <JANA/JException.h>
#include <JANA/Utils/JAny.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JCallGraphRecorder.h>
#include <JANA/Omni/JComponent.h>
#include <JANA/Omni/JCollection.h>

#include <string>
#include <typeindex>
#include <memory>
#include <limits>
#include <atomic>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <functional>


class JEvent;
class JObject;
class JApplication;

class JFactory : public jana::omni::JComponent {
public:

    enum class Status {Uninitialized, Unprocessed, Processed, Inserted};
    enum class CreationStatus { NotCreatedYet, Created, Inserted, InsertedViaGetObjects, NeverCreated };

    JFactory() = default;

    JFactory(std::string aName, std::string aTag = "")
    : mObjectName(std::move(aName)), 
      mTag(std::move(aTag)), 
      mStatus(Status::Uninitialized) {
          SetPrefix(aTag.empty() ? mObjectName : mObjectName + ":" + mTag);
    };

    virtual ~JFactory() {
        for (JCollection* collection : mCollections) {
            delete collection;
        }
    }


    std::string GetName() const __attribute__ ((deprecated))  { return mObjectName; }

    std::string GetTag() const { return mTag; }
    std::string GetObjectName() const { return mObjectName; }
    std::string GetFactoryName() const { return m_type_name; }
    Status GetStatus() const { return mStatus; }
    CreationStatus GetCreationStatus() const { return mCreationStatus; }
    JCallGraphRecorder::JDataOrigin GetInsertOrigin() const { return m_insert_origin; } ///< If objects were placed here by JEvent::Insert() this records whether that call was made from a source or factory.

    uint32_t GetPreviousRunNumber(void) const { return mPreviousRunNumber; }

    std::vector<JCollection*>& GetCollections() { return mCollections; }
    bool GetRegenerateFlag() const { return m_regenerate; }

    void SetName(std::string objectName) __attribute__ ((deprecated)) { mObjectName = std::move(objectName); }

    void SetTag(std::string tag) { mTag = std::move(tag); }
    void SetObjectName(std::string objectName) { mObjectName = std::move(objectName); }
    void SetFactoryName(std::string factoryName) { SetTypeName(factoryName); }

    void SetStatus(Status status){ mStatus = status; }
    void SetCreationStatus(CreationStatus status){ mCreationStatus = status; }
    void SetInsertOrigin(JCallGraphRecorder::JDataOrigin origin) { m_insert_origin = origin; } ///< Called automatically by JEvent::Insert() to records whether that call was made by a source or factory.

    void SetPreviousRunNumber(uint32_t aRunNumber) { mPreviousRunNumber = aRunNumber; }
    void SetRegenerateFlag(bool regenerate) { m_regenerate = regenerate; }

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

    virtual void ClearData() {};


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
    /// type of object contained. In order to access these objects when all you have is a JFactory*, use JFactory::GetAs().
    virtual void Create(const std::shared_ptr<const JEvent>& event);
    void DoInit();
    void Summarize(JComponentSummary& summary);


    virtual void Set(const std::vector<JObject *> &data) { throw JException("Unsupported!"); }
    virtual void Insert(JObject *data) { throw JException("Unsupported!"); }


protected:

    std::vector<JCollection*> mCollections;
    std::string mObjectName;
    std::string mTag;
    int32_t mPreviousRunNumber = -1;
    bool m_regenerate = false;

    mutable Status mStatus = Status::Uninitialized;
    mutable JCallGraphRecorder::JDataOrigin m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE; // (see note at top of JCallGraphRecorder.h)

    CreationStatus mCreationStatus = CreationStatus::NotCreatedYet;
};

template<typename S>
std::vector<S*> JFactory::GetAs() {
    return mCollections[0]->GetAs<S>();
}


