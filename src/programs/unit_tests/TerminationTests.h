
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_TERMINATIONTESTS_H
#define JANA2_TERMINATIONTESTS_H

#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

#include "catch.hpp"

struct InterruptedSource : public JEventSource {
    InterruptedSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void Open() override { GetApplication()->Stop(); }
    Result Emit(JEvent&) override { return Result::Success; }
};

struct BoundedSource : public JEventSource {

    uint64_t event_count = 0;

    BoundedSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Open() override {
    }

    Result Emit(JEvent&) override {
        if (event_count >= 10) {
            return Result::FailureFinished;
        }
        event_count += 1;
        return Result::Success;
    }
};

struct UnboundedSource : public JEventSource {

    uint64_t event_count = 0;

    UnboundedSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Open() override {
    }

    Result Emit(JEvent& event) override {
        event_count += 1;
        event.SetEventNumber(event_count);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return Result::Success;
    }
};

struct CountingProcessor : public JEventProcessor {

    std::atomic_int processed_count {0};
    std::atomic_int finish_call_count {0};

    CountingProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Init() override {}

    void Process(const JEvent&) override {
        processed_count += 1;
        // jout << "Processing " << event->GetEventNumber() << jendl;
        REQUIRE(finish_call_count == 0);
    }

    void Finish() override {
        finish_call_count += 1;
    }
};


#endif //JANA2_TERMINATIONTESTS_H
