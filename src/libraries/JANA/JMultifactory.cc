
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JMultifactory.h"
#include <JANA/JEvent.h>


void JMultifactory::Execute(const std::shared_ptr<const JEvent>& event) {

    std::lock_guard<std::mutex> lock(m_mutex);
#ifdef JANA2_HAVE_PODIO
    if (mNeedPodio) {
        mPodioFrame = GetOrCreateFrame(event);
    }
#endif

    if (m_status == Status::Uninitialized) {
        try {
            Init();
            m_status = Status::Initialized;
        }
        catch(std::exception &e) {
            // Rethrow as a JException so that we can add context information
            throw JException(e.what());
        }
    }

    auto run_number = event->GetRunNumber();
    if (m_last_run_number == -1) {
        // This is the very first run
        try {
            BeginRun(event);
            m_last_run_number = run_number;
        }
        catch(std::exception &e) {
            // Rethrow as a JException so that we can add context information
            throw JException(e.what());
        }
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
            m_last_run_number = run_number;
        }
        catch(std::exception &e) {
            // Rethrow as a JException so that we can add context information
            throw JException(e.what());
        }
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
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        // Only call Finish() if we actually initialized
        // Only call Finish() once
        if (m_status == Status::Initialized) {
            Finish();
            m_status = Status::Finalized;
        }
    }
    catch(std::exception &e) {
        // Rethrow as a JException so that we can add context information
        throw JException(e.what());
    }
}

JFactorySet* JMultifactory::GetHelpers() {
    return &mHelpers;
}
