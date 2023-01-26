
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JEventCallback.h"
#include "JEvent.h"
#include <memory>

namespace jana2 {

void JEventCallback::Release() {
    std::call_once(m_is_finished, &JEventCallback::Finish, this);
}


void JEventCallback::Execute(const std::shared_ptr<const JEvent> &event) {

    auto run_number = event->GetRunNumber();
    std::call_once(m_is_initialized, &JEventCallback::Init, this);

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

} // namespace jana2

