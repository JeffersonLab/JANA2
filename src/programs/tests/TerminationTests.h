
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_TERMINATIONTESTS_H
#define JANA2_TERMINATIONTESTS_H

#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

#include "catch.hpp"

struct InterruptedSource : public JEventSource {
    InterruptedSource(std::string source_name, JApplication* app) : JEventSource(source_name, app) {}
    static std::string GetDescription() { return "ComponentTests Fake Event Source"; }
    std::string GetType(void) const override { return JTypeInfo::demangle<decltype(*this)>(); }
    void Open() override { GetApplication()->Stop(); }
    void GetEvent(std::shared_ptr<JEvent>) override {}
};

struct BoundedSource : public JEventSource {

    uint64_t event_count = 0;

    BoundedSource(std::string source_name, JApplication *app) : JEventSource(source_name, app)
    { }

    static std::string GetDescription() {
        return "ComponentTests Fake Event Source";
    }

    std::string GetType(void) const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }

    void Open() override {
    }

    void GetEvent(std::shared_ptr<JEvent>) override {
        if (event_count >= 10) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
        event_count += 1;
    }
};

struct UnboundedSource : public JEventSource {

    uint64_t event_count = 0;

    UnboundedSource(std::string source_name, JApplication *app) : JEventSource(source_name, app)
    { }

    static std::string GetDescription() {
        return "ComponentTests Fake Event Source";
    }

    std::string GetType(void) const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }

    void Open() override {
    }

    void GetEvent(std::shared_ptr<JEvent> event) override {
        event_count += 1;
        event->SetEventNumber(event_count);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // jout << "Emitting " << event_count << jendl;
    }
};

struct CountingProcessor : public JEventProcessor {

    std::atomic_int processed_count {0};
    std::atomic_int finish_call_count {0};


    CountingProcessor(JApplication* app) : JEventProcessor(app) {}

    void Init() override {}

    void Process(const std::shared_ptr<const JEvent>& /*event*/) override {
        processed_count += 1;
        // jout << "Processing " << event->GetEventNumber() << jendl;
        REQUIRE(finish_call_count == 0);
    }

    void Finish() override {
        finish_call_count += 1;
    }
};


#endif //JANA2_TERMINATIONTESTS_H
