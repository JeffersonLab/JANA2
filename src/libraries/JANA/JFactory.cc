
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JTypeInfo.h>


void JFactory::Create(const std::shared_ptr<const JEvent>& event) {

    if (m_app == nullptr && event->GetJApplication() != nullptr) {
        // These are usually set by JFactoryGeneratorT, but some user code has custom JFactoryGenerators which don't!
        // The design of JFactoryGenerator doesn't give us a better place to inject things
        m_app = event->GetJApplication();
        m_logger = m_app->GetJParameterManager()->GetLogger(GetLoggerName());
    }

    if (mStatus == Status::Uninitialized) {
        DoInit();
    }

    // How do we obtain our data? The priority is as follows:
    // 1. JFactory::Process() if REGENERATE flag is set
    // 2. JEvent::Insert()
    // 3. JEventSource::GetObjects() if source has GetObjects() enabled
    // 4. JFactory::Process()

    // ---------------------------------------------------------------------
    // 1. JFactory::Process() if REGENERATE flag is set
    // ---------------------------------------------------------------------

    if (TestFactoryFlag(REGENERATE)) {
        if (mStatus == Status::Inserted) {
            // Status::Inserted indicates that the data came from either src->GetObjects() or evt->Insert()
            ClearData(); 
            // ClearData() resets mStatus to Unprocessed so that the data will be regenerated exactly once.
        }
        // After this point, control flow falls through to "4. JFactory::Process"
    }
    else {

        // ---------------------------------------------------------------------
        // 2. JEvent::Insert()
        // ---------------------------------------------------------------------

        if (mStatus == Status::Inserted) {
            // This may include data cached from eventsource->GetObjects().
            // Either way, short-circuit here, because the data is present.
            return;
        }

        // ---------------------------------------------------------------------
        // 3. JEventSource::GetObjects() if source has GetObjects() enabled
        // ---------------------------------------------------------------------

        auto src = event->GetJEventSource();
        if (src != nullptr && src->IsGetObjectsEnabled()) {
            bool found_data = false;

            CallWithJExceptionWrapper("JEventSource::GetObjects", [&](){ 
                found_data = src->GetObjects(event, this); });

            if (found_data) {
                mStatus = Status::Inserted;
                mCreationStatus = CreationStatus::InsertedViaGetObjects;
                return;
            }
        }
        // If neither "2. JEvent::Insert()" nor "3. JEventSource::GetObjects()" succeeded, fall through to "4. JFactory::Process()"
    }

    // ---------------------------------------------------------------------
    // 4. JFactory::Process()
    // ---------------------------------------------------------------------

    // If the data was Processed (instead of Inserted), it will be in cache, and we can just exit.
    // Otherwise we call Process() to create the data in the first place.
    if (mStatus == Status::Unprocessed) {
        auto run_number = event->GetRunNumber();
        if (mPreviousRunNumber != run_number) {
            if (mPreviousRunNumber != -1) {
                CallWithJExceptionWrapper("JFactory::EndRun", [&](){ EndRun(); });
            }
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
    if (mStatus != Status::Uninitialized) {
        return;
    }
    for (auto* parameter : m_parameters) {
        parameter->Configure(*(m_app->GetJParameterManager()), m_prefix);
    }
    for (auto* service : m_services) {
        service->Fetch(m_app);
    }
    CallWithJExceptionWrapper("JFactory::Init", [&](){ Init(); });
    mStatus = Status::Unprocessed;
}

void JFactory::DoFinish() {
    if (mStatus == Status::Unprocessed || mStatus == Status::Processed) {
        if (mPreviousRunNumber != -1) {
            CallWithJExceptionWrapper("JFactory::EndRun", [&](){ EndRun(); });
        }
        CallWithJExceptionWrapper("JFactory::Finish", [&](){ Finish(); });
        mStatus = Status::Finished;
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


