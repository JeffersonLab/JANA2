
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JTypeInfo.h>


class FlagGuard {
    bool* m_flag;
public:
    FlagGuard(bool* flag) : m_flag(flag) {
        *m_flag = true;
    }
    ~FlagGuard() {
        *m_flag = false;
    }
};

void JFactory::Create(const std::shared_ptr<const JEvent>& event) {
    Create(*event.get());
}

void JFactory::Create(const JEvent& event) {

    if (mInsideCreate && (mStatus != Status::Inserted)) {
        // Ideally, we disallow any calls to Create() that end up calling it right back. However, we do allow
        // calls that go down to GetObjects, who inserts the data, but then RETRIEVES the same data it just inserted,
        // so that it can subsequently calculate and insert OTHER data. Once we refactor JEventSourceEVIOpp, we can consider
        // removing this weird edge case.
        throw JException("Encountered a cycle in the factory dependency graph! Hint: Maybe this data was supposed to be inserted in the JEventSource");
    }
    FlagGuard insideCreateFlagGuard (&mInsideCreate); // No matter how we exit from Create() (particularly with exceptions) mInsideCreate will be set back to false

    if (m_app == nullptr && event.GetJApplication() != nullptr) {
        // These are usually set by JFactoryGeneratorT, but some user code has custom JFactoryGenerators which don't!
        // The design of JFactoryGenerator doesn't give us a better place to inject things
        m_app = event.GetJApplication();
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

    if (mRegenerate) {
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

        auto src = event.GetJEventSource();
        if (src != nullptr && src->IsGetObjectsEnabled()) {
            bool found_data = false;

            CallWithJExceptionWrapper("JEventSource::GetObjects", [&](){ 
                found_data = src->GetObjects(event.shared_from_this(), this); });

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
    // If we already ran Process() but it excepted, we re-run Process() to trigger the same exception, so that every consumer
    // is forced to handle it. Otherwise one "fault-tolerant" consumer will swallow the exception for everybody else.
    if (mStatus == Status::Unprocessed || mStatus == Status::Excepted) {
        auto run_number = event.GetRunNumber();
        if (mPreviousRunNumber != run_number) {
            if (mPreviousRunNumber != -1) {
                CallWithJExceptionWrapper("JFactory::EndRun", [&](){ EndRun(); });
            }
            CallWithJExceptionWrapper("JFactory::ChangeRun", [&](){ ChangeRun(event.shared_from_this()); });
            CallWithJExceptionWrapper("JFactory::BeginRun", [&](){ BeginRun(event.shared_from_this()); });
            mPreviousRunNumber = run_number;
        }
        try {
            CallWithJExceptionWrapper("JFactory::Process", [&](){ Process(event.shared_from_this()); });
            mStatus = Status::Processed;
            mCreationStatus = CreationStatus::Created;
            for (auto* output : GetDatabundleOutputs()) {
                output->StoreData(*event.GetFactorySet(), JDatabundle::Status::Created);
            }
        }
        catch (...) {
            // Save everything already created even if we throw an exception
            // This is so that we leave everything in a valid state just in case someone tries to catch the exception recover,
            // such as EICrecon. (Remember that a missing collection in the podio frame will segfault if anyone tries to write that frame)
            // Note that the collections themselves won't know that they exited early

            LOG << "Exception in JFactory::Create, prefix=" << GetPrefix();
            mStatus = Status::Excepted;
            mCreationStatus = CreationStatus::Created;
            for (auto* output : GetDatabundleOutputs()) {
                output->StoreData(*event.GetFactorySet(), JDatabundle::Status::Excepted);
            }
            throw;
        }

    }
}

void JFactory::DoInit() {
    if (mStatus != Status::Uninitialized) {
        return;
    }
    for (auto* parameter : m_parameters) {
        parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
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


