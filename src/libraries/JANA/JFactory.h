
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

    enum JFactory_Flags_t {
        JFACTORY_NULL = 0x00,    // Not used anywhere
        PERSISTENT = 0x01,       // Used heavily. Possibly better served by JServices, hierarchical events, or event groups. 
        WRITE_TO_OUTPUT = 0x02,  // Set in halld_recon but not read except by JANA1 janaroot and janacontrol plugins
        NOT_OBJECT_OWNER = 0x04, // Used heavily. Small conflict with PODIO subset collections, which do the same thing at a different level
        REGENERATE = 0x08        // Replaces JANA1 JFactory_base::use_factory and JFactory::GetCheckSourceFirst()
    };

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

    void SetName(std::string objectName) __attribute__ ((deprecated)) { mObjectName = std::move(objectName); }

    void SetTag(std::string tag) { mTag = std::move(tag); }
    void SetObjectName(std::string objectName) { mObjectName = std::move(objectName); }
    void SetFactoryName(std::string factoryName) { SetTypeName(factoryName); }

    void SetStatus(Status status){ mStatus = status; }
    void SetCreationStatus(CreationStatus status){ mCreationStatus = status; }
    void SetInsertOrigin(JCallGraphRecorder::JDataOrigin origin) { m_insert_origin = origin; } ///< Called automatically by JEvent::Insert() to records whether that call was made by a source or factory.

    void SetPreviousRunNumber(uint32_t aRunNumber) { mPreviousRunNumber = aRunNumber; }

    // Note: JFactory_Flags_t is going to be deprecated. Use Set...Flag()s instead
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

    inline void SetPersistentFlag(bool persistent) { 
        if (persistent) { 
            SetFactoryFlag(PERSISTENT); }
        else { 
            ClearFactoryFlag(PERSISTENT); }
    }

    inline void SetNotOwnerFlag(bool not_owner) { 
        if (not_owner) {
            SetFactoryFlag(NOT_OBJECT_OWNER); }
        else { 
            ClearFactoryFlag(NOT_OBJECT_OWNER); }
    }

    inline void SetRegenerateFlag(bool regenerate) { 
        if (regenerate) {
            SetFactoryFlag(REGENERATE); }
        else { 
            ClearFactoryFlag(REGENERATE); }
    }

    inline void SetWriteToOutputFlag(bool write_to_output) { 
        if (write_to_output) {
            SetFactoryFlag(WRITE_TO_OUTPUT); }
        else {
            ClearFactoryFlag(WRITE_TO_OUTPUT); }
    }

    inline bool GetWriteToOutputFlag() { 
        return TestFactoryFlag(WRITE_TO_OUTPUT);
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
    /// type of object contained. In order to access these objects when all you have is a JFactory*, use JFactory::GetAs().
    virtual void Create(const std::shared_ptr<const JEvent>& event);
    void DoInit();
    void Summarize(JComponentSummary& summary);


    virtual void Set(const std::vector<JObject *> &data) = 0;
    virtual void Insert(JObject *data) = 0;


protected:

    std::vector<JCollection*> mCollections;
    std::string mObjectName;
    std::string mTag;
    uint32_t mFlags = WRITE_TO_OUTPUT;
    int32_t mPreviousRunNumber = -1;
    std::unordered_map<std::type_index, std::unique_ptr<JAny>> mUpcastVTable;

    mutable Status mStatus = Status::Uninitialized;
    mutable JCallGraphRecorder::JDataOrigin m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE; // (see note at top of JCallGraphRecorder.h)

    CreationStatus mCreationStatus = CreationStatus::NotCreatedYet;
};

template<typename S>
std::vector<S*> JFactory::GetAs() {
    return mCollections[0]->GetAs<S>();
}


