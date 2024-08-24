
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JTypeInfo.h>


void JFactory::Create(const std::shared_ptr<const JEvent>& event) {

    if (mStatus == Status::Uninitialized) {
        CallWithJExceptionWrapper("JFactory::Init", [&](){ Init(); });
        mStatus = Status::Unprocessed;
    }

    auto src = event->GetJEventSource();
    if (!TestFactoryFlag(REGENERATE) && src != nullptr && src->IsGetObjectsEnabled()) {
        // Attempt to obtain factory data via source->GetObjects(). This will eventually be deprecated,
        // but for now we want it for migration purposes. If GetObjects() is not implemented, the default
        // implementation returns false with a minimal performance penalty.
        bool found_data = false;

        CallWithJExceptionWrapper("JEventSource::GetObjects", [&](){ 
            found_data = src->GetObjects(event, this); });

        if (found_data) {
            mStatus = Status::Inserted;
            mCreationStatus = CreationStatus::InsertedViaGetObjects;
            return;
        }
    }

    if (TestFactoryFlag(REGENERATE) && (mStatus == Status::Inserted)) {
        ClearData();
        // ClearData will reset mStatus to Status::Unprocessed
    }

    if (mStatus == Status::Unprocessed) {
        auto run_number = event->GetRunNumber();
        if (mPreviousRunNumber == -1) {
            // This is the very first run
            CallWithJExceptionWrapper("JFactory::ChangeRun", [&](){ ChangeRun(event); });
            CallWithJExceptionWrapper("JFactory::BeginRun", [&](){ BeginRun(event); });
            mPreviousRunNumber = run_number;
        }
        else if (mPreviousRunNumber != run_number) {
            // This is a later run, and it has changed
            CallWithJExceptionWrapper("JFactory::EndRun", [&](){ EndRun(); });
            CallWithJExceptionWrapper("JFactory::ChangeRun", [&](){ ChangeRun(event); });
            CallWithJExceptionWrapper("JFactory::BeginRun", [&](){ BeginRun(event); });
            mPreviousRunNumber = run_number;
        }
        CallWithJExceptionWrapper("JFactory::Process", [&](){ Process(event); });
        for (auto& output : this->GetOutputs()) {
            output->PutCollections(*event);
        }
        mStatus = Status::Processed;
        mCreationStatus = CreationStatus::Created;
    }
}

void JFactory::DoInit() {
    if (mStatus == Status::Uninitialized) {
        CallWithJExceptionWrapper("JFactory::Init", [&](){ Init(); });
        mStatus = Status::Unprocessed;
    }
}

void JFactory::Summarize(JComponentSummary& summary) const {

    auto fs = new JComponentSummary::Component(
            "Factory",
            GetPrefix(),
            GetTypeName(),
            GetLevel(),
            GetPluginName());

    auto coll = new JComponentSummary::Collection(
            GetTag(), 
            GetTag(),
            GetObjectName(), 
            GetLevel());

    fs->AddOutput(coll);
    summary.Add(fs);
}


