
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once 

#include <JANA/JException.h>
#include <JANA/Utils/JAny.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JCallGraphRecorder.h>
#include <JANA/Components/JComponent.h>
#include <JANA/Components/JHasDatabundleOutputs.h>

#include <string>
#include <typeindex>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>


class JEvent;
class JObject;
class JApplication;

class JFactory : public jana::components::JComponent, 
                 public jana::components::JHasDatabundleOutputs {
public:

    enum class Status {Uninitialized, Unprocessed, Processed, Inserted, Finished};
    enum class CreationStatus { NotCreatedYet, Created, Inserted, InsertedViaGetObjects, NeverCreated };

    enum JFactory_Flags_t {
        JFACTORY_NULL = 0x00,    // Not used anywhere
        PERSISTENT = 0x01,       // Used heavily. Possibly better served by JServices, hierarchical events, or event groups. 
        WRITE_TO_OUTPUT = 0x02,  // Set in halld_recon but not read except by JANA1 janaroot and janacontrol plugins
        NOT_OBJECT_OWNER = 0x04, // Used heavily. Small conflict with PODIO subset collections, which do the same thing at a different level
        REGENERATE = 0x08        // Replaces JANA1 JFactory_base::use_factory and JFactory::GetCheckSourceFirst()
    };

    JFactory() = default;
    virtual ~JFactory() = default;

    std::string GetTag() const { 
        auto& db = GetDatabundleOutputs().at(0)->GetDatabundles().at(0);
        if (db->HasShortName()) {
            return db->GetShortName();
        }
        return db->GetUniqueName();
    }

    std::string GetObjectName() const { return mObjectName; }

    void SetObjectName(std::string object_name) { mObjectName = object_name; }

    std::string GetFactoryName() const { return m_type_name; }
    Status GetStatus() const { return mStatus; }
    CreationStatus GetCreationStatus() const { return mCreationStatus; }
    JCallGraphRecorder::JDataOrigin GetInsertOrigin() const { return m_insert_origin; } ///< If objects were placed here by JEvent::Insert() this records whether that call was made from a source or factory.

    uint32_t GetPreviousRunNumber(void) const { return mPreviousRunNumber; }

    void SetFactoryName(std::string factoryName) { SetTypeName(factoryName); }
    void SetStatus(Status status){ mStatus = status; }
    void SetCreationStatus(CreationStatus status){ mCreationStatus = status; }
    void SetInsertOrigin(JCallGraphRecorder::JDataOrigin origin) { m_insert_origin = origin; } ///< Called automatically by JEvent::Insert() to records whether that call was made by a source or factory.

    void SetPreviousRunNumber(uint32_t aRunNumber) { mPreviousRunNumber = aRunNumber; }

    virtual void SetFactoryFlag(JFactory_Flags_t f) {
        switch (f) {
            case JFactory::REGENERATE: SetRegenerateFlag(false); break;
            case JFactory::WRITE_TO_OUTPUT: SetWriteToOutputFlag(false); break;
            default: throw JException("Unsupported factory flag");
        }
    };
    void SetRegenerateFlag(bool regenerate) { mRegenerate = regenerate; }
    void SetWriteToOutputFlag(bool write_to_output) { mWriteToOutput = write_to_output; }
    bool GetWriteToOutputFlag() { return mWriteToOutput; }

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
    virtual void Create(const JEvent& event);

    void DoInit();
    void DoFinish();
    void Summarize(JComponentSummary& summary) const override;


    virtual void Set(const std::vector<JObject *> &data) = 0;
    virtual void Insert(JObject *data) = 0;


protected:

    bool mRegenerate = false;
    bool mWriteToOutput = true;
    int32_t mPreviousRunNumber = -1;
    bool mInsideCreate = false; // Use this to detect cycles in factory dependencies
    std::unordered_map<std::type_index, std::unique_ptr<JAny>> mUpcastVTable;
    std::string mObjectName;

    mutable Status mStatus = Status::Uninitialized;
    mutable JCallGraphRecorder::JDataOrigin m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE; // (see note at top of JCallGraphRecorder.h)

    CreationStatus mCreationStatus = CreationStatus::NotCreatedYet;
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


