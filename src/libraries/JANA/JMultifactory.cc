
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JMultifactory.h"
#include <JANA/JEvent.h>


void JMultifactory::Execute(const std::shared_ptr<const JEvent>& event) {

    auto run_number = event->GetRunNumber();
    std::call_once(m_is_initialized, &JMultifactory::Init, this);
    if (m_last_run_number == -1) {
        // This is the very first run
        BeginRun(event);
        m_last_run_number = run_number;
    }
    else if (m_last_run_number != run_number) {
        // This is a later run, and it has changed
        EndRun();
        BeginRun(event);
        m_last_run_number = run_number;
    }
    Process(event);
}

void JMultifactory::Release() {
    std::call_once(m_is_finished, &JMultifactory::Finish, this);
}

JFactorySet* JMultifactory::GetHelpers() {
    return &mHelpers;
}
