
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JMultifactory.h"
#include <JANA/JEvent.h>


void JMultifactory::Execute(const std::shared_ptr<const JEvent>& event) {

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status == Status::Uninitialized) {
        CallWithJExceptionWrapper("JMultifactory::Init", [&](){
            Init();
        });
        m_status = Status::Initialized;
    }

    auto run_number = event->GetRunNumber();
    if (m_last_run_number == -1) {
        // This is the very first run
        CallWithJExceptionWrapper("JMultifactory::BeginRun", [&](){
            BeginRun(event);
        });
        m_last_run_number = run_number;
    }
    else if (m_last_run_number != run_number) {
        // This is a later run, and it has changed
        CallWithJExceptionWrapper("JMultifactory::EndRun", [&](){
            EndRun();
        });
        CallWithJExceptionWrapper("JMultifactory::BeginRun", [&](){
            BeginRun(event);
        });
        m_last_run_number = run_number;
    }
    CallWithJExceptionWrapper("JMultifactory::Process", [&](){
        Process(event);
    });
}

void JMultifactory::Release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Only call Finish() if we actually initialized
    // Only call Finish() once
    if (m_status == Status::Initialized) {
        CallWithJExceptionWrapper("JMultifactory::Finish", [&](){
            Finish();
        });
        m_status = Status::Finalized;
    }
}


JFactorySet* JMultifactory::GetHelpers() {
    return &mHelpers;
}

void JMultifactory::DoInit() {

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status == Status::Uninitialized) {
        CallWithJExceptionWrapper("JMultifactory::Init", [&](){
            Init();
        });
        m_status = Status::Initialized;
    }
}

void JMultifactory::Summarize(JComponentSummary& summary) const {
    auto mfs = new JComponentSummary::Component(
            "Multifactory",
            GetPrefix(),
            GetTypeName(),
            GetLevel(),
            GetPluginName());

    for (auto* helper : mHelpers.GetAllFactories()) {
        auto coll = new JComponentSummary::Collection(
                helper->GetTag(), 
                helper->GetFactoryName(), 
                helper->GetObjectName(), 
                helper->GetLevel());
        mfs->AddOutput(coll);
    }
    summary.Add(mfs);
}
