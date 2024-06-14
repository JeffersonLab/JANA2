
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once 

#include <JANA/JException.h>
#include <JANA/Utils/JCallGraphRecorder.h>
#include <JANA/Omni/JComponent.h>
#include <JANA/Omni/JCollection.h>

class JEvent;

class JFactory : public jana::omni::JComponent {

public:
    enum class Status {Uninitialized, Unprocessed, Processed, Inserted, Finalized};
    enum class CreationStatus { NotCreatedYet, Created, Inserted, InsertedViaGetObjects };

protected:
    std::vector<JCollection*> mCollections;
    int32_t mPreviousRunNumber = -1;
    bool m_regenerate = false;
    mutable Status mStatus = Status::Uninitialized;
    CreationStatus mCreationStatus = CreationStatus::NotCreatedYet;
    mutable JCallGraphRecorder::JDataOrigin m_insert_origin = JCallGraphRecorder::ORIGIN_NOT_AVAILABLE; // (see note at top of JCallGraphRecorder.h)

#ifdef JANA2_HAVE_PODIO
    bool mNeedPodio = false;      // Whether we need to retrieve the podio::Frame
    podio::Frame* mPodioFrame = nullptr;  // To provide the podio::Frame to SetPodioData, SetCollection
#endif


public:
    JFactory() = default;

    virtual ~JFactory() {
        for (JCollection* collection : mCollections) {
            delete collection;
        }
    }

    std::vector<JCollection*>& GetCollections() { return mCollections; }

    uint32_t GetPreviousRunNumber(void) const { return mPreviousRunNumber; }
    bool GetRegenerateFlag() { return m_regenerate; }
    Status GetStatus() const { return mStatus; }
    CreationStatus GetCreationStatus() const { return mCreationStatus; }
    /// If objects were placed here by JEvent::Insert(), record whether that call was made from a source or factory.
    JCallGraphRecorder::JDataOrigin GetInsertOrigin() const { return m_insert_origin; } 

    void SetStatus(Status status){ mStatus = status; }
    void SetCreationStatus(CreationStatus status){ mCreationStatus = status; }
    /// Called automatically by JEvent::Insert() to records whether that call was made by a source or factory.
    void SetInsertOrigin(JCallGraphRecorder::JDataOrigin origin) { m_insert_origin = origin; } 
    void SetPreviousRunNumber(uint32_t aRunNumber) { mPreviousRunNumber = aRunNumber; }
    void SetRegenerateFlag(bool regenerate) { m_regenerate = regenerate; }


    // Called by JANA

    // Create() calls JFactory::Init,BeginRun,Process in an invariant-preserving way
    virtual void Create(const std::shared_ptr<const JEvent>& event);

    // ClearData() clears all collections
    virtual void ClearData() { for (JCollection* c : mCollections) c->ClearData(); };

    // Release() makes sure Finish() is called exactly once
    void Release();

    // Implemented by user
    virtual void Init() {}
    virtual void BeginRun(const std::shared_ptr<const JEvent>&) {}
    virtual void ChangeRun(const std::shared_ptr<const JEvent>&) {}
    virtual void EndRun() {}
    virtual void Process(const std::shared_ptr<const JEvent>&) {}
    virtual void Finish() {}

};


