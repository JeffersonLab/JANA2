
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JMultifactory.h"
#include <JANA/JEvent.h>


void JMultifactory::Execute(const std::shared_ptr<const JEvent>& event) {

#ifdef JANA2_HAVE_PODIO
    if (mNeedPodio) {
        mPodioFrame = GetOrCreateFrame(event);
    }
#endif

    auto run_number = event->GetRunNumber();
    try {
        std::call_once(m_is_initialized, &JMultifactory::Init, this);
    }
    catch(std::exception &e) {
        // Rethrow as a JException so that we can add context information
        throw JException(e.what());
    }

    if (m_last_run_number == -1) {
        // This is the very first run
        try {
            BeginRun(event);
        }
        catch(std::exception &e) {
            // Rethrow as a JException so that we can add context information
            throw JException(e.what());
        }
        m_last_run_number = run_number;
    }
    else if (m_last_run_number != run_number) {
        // This is a later run, and it has changed
        try {
            EndRun();
        }
        catch(std::exception &e) {
            // Rethrow as a JException so that we can add context information
            throw JException(e.what());
        }
        try {
            BeginRun(event);
        }
        catch(std::exception &e) {
            // Rethrow as a JException so that we can add context information
            throw JException(e.what());
        }
        m_last_run_number = run_number;
    }
    try {
        Process(event);
    }
    catch(std::exception &e) {
        // Rethrow as a JException so that we can add context information
        throw JException(e.what());
    }
}

void JMultifactory::Release() {
    try {
        std::call_once(m_is_finished, &JMultifactory::Finish, this);
    }
    catch(std::exception &e) {
        // Rethrow as a JException so that we can add context information
        throw JException(e.what());
    }
}

JFactorySet* JMultifactory::GetHelpers() {
    return &mHelpers;
}
