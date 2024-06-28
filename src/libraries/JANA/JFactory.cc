
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JTypeInfo.h>


void JFactory::Create(const std::shared_ptr<const JEvent>& event) {

    if (mStatus == Status::Uninitialized) {
        CallWithJExceptionWrapper("JFactory::Init", [&](){ Init(); });
        mStatus = Status::Unprocessed;
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
        mStatus = Status::Processed;
        mCreationStatus = CreationStatus::Created;
    }
}

void JFactory::DoInit() {
    if (GetApplication() == nullptr) {
        throw JException("JFactory::DoInit(): Null JApplication pointer");
    }
    if (mStatus == Status::Uninitialized) {
        CallWithJExceptionWrapper("JFactory::Init", [&](){ Init(); });
        mStatus = Status::Unprocessed;
    }
}

void JFactory::Summarize(JComponentSummary& summary) {
    summary.factories.push_back({
        .level = GetLevel(),
        .plugin_name = GetPluginName(),
        .factory_name = GetFactoryName(),
        .factory_tag = GetTag(),
        .object_name = GetObjectName()
    });
}
