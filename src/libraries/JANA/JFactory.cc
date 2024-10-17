
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
        mStatus = Status::Processed;
        mCreationStatus = CreationStatus::Created;
    }
}

void JFactory::DoInit() {
    if (mStatus != Status::Uninitialized) {
        throw JException("Attempted to initialize a JFactory that has already been initialized!");
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


