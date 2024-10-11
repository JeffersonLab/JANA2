
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <JANA/JEventSource.h>


struct NEventNSkipBoundedSource : public JEventSource {

    std::atomic_int event_count {0};
    int event_bound = 100;
    std::vector<int> events_emitted;
    std::atomic_int open_count{0};
    std::atomic_int close_count{0};

    NEventNSkipBoundedSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    Result Emit(JEvent&) override {
        if (event_count >= event_bound) {
            return Result::FailureFinished;
        }
        event_count += 1;
        events_emitted.push_back(event_count);
        return Result::Success;
    }

    void Open() override {
        open_count++;
        LOG << "Opening source " << GetResourceName() << LOG_END;
    }
    void Close() override {
        close_count++;
        LOG << "Closing source " << GetResourceName() << LOG_END;
    }
};


TEST_CASE("NEventNSkipTests") {

    JApplication app;
    auto source = new NEventNSkipBoundedSource();
    app.Add(source);

    SECTION("[1..100] @ nskip=0, nevents=0 => [1..100]") {

        app.SetParameterValue("jana:nskip", 0);
        app.SetParameterValue("jana:nevents", 0);
        app.SetParameterValue("nthreads", 1);
        app.Run(true);
        REQUIRE(source->event_count == 100);
        REQUIRE(source->events_emitted.size() == 100);
        REQUIRE(source->events_emitted[0] == 1);
        REQUIRE(source->events_emitted[99] == 100);
    }

    SECTION("[1..100] @ nskip=0, nevents=22 => [1..22]") {

        app.SetParameterValue("jana:nskip", 0);
        app.SetParameterValue("jana:nevents", 22);
        app.SetParameterValue("nthreads", 1);
        app.Run(true);
        REQUIRE(source->event_count == 22);
        REQUIRE(source->events_emitted.size() == 22);
        REQUIRE(source->events_emitted[0] == 1);
        REQUIRE(source->events_emitted[21] == 22);
    }

    SECTION("[1..100] @ nskip=30, nevents=20 => [31..51]") {

        app.SetParameterValue("jana:nskip", 30);
        app.SetParameterValue("jana:nevents", 20);
        app.SetParameterValue("nthreads", 1);
        app.Run(true);
        REQUIRE(source->event_count == 50);
        // All 50 events get filled, but the first 30 are discarded without emitting
        // However, all 50 go into events_emitted
    }
}

TEST_CASE("JEventSourceArrow with multiple JEventSources") {
    JApplication app;
    auto source1 = new NEventNSkipBoundedSource();
    auto source2 = new NEventNSkipBoundedSource();
    auto source3 = new NEventNSkipBoundedSource();
    app.Add(source1);
    app.Add(source2);
    app.Add(source3);

    SECTION("All three event sources initialize, run, and finish") {
        source1->event_bound = 9;
        source2->event_bound = 13;
        source3->event_bound = 7;

        app.SetParameterValue("jana:nskip", 0);
        app.SetParameterValue("jana:nevents", 0);
        app.SetParameterValue("nthreads", 4);
        app.Run(true);

        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Success);
        REQUIRE(source1->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source2->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source3->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source1->open_count == 1);
        REQUIRE(source2->open_count == 1);
        REQUIRE(source3->open_count == 1);
        REQUIRE(source1->close_count == 1);
        REQUIRE(source2->close_count == 1);
        REQUIRE(source3->close_count == 1);
        REQUIRE(source1->GetEmittedEventCount() == 9);
        REQUIRE(source2->GetEmittedEventCount() == 13);
        REQUIRE(source3->GetEmittedEventCount() == 7);
        REQUIRE(app.GetNEventsProcessed() == 9+13+7);
    }

    SECTION("All three event sources initialize, run, and finish, each using the same nskip,nevents (self-terminated)") {
        source1->event_bound = 9;
        source2->event_bound = 13;
        source3->event_bound = 7;

        app.SetParameterValue("jana:nskip", 3);
        app.SetParameterValue("jana:nevents", 9);
        app.SetParameterValue("nthreads", 4);
        app.Run(true);

        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Success);
        REQUIRE(source1->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source2->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source3->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source1->open_count == 1);
        REQUIRE(source2->open_count == 1);
        REQUIRE(source3->open_count == 1);
        REQUIRE(source1->close_count == 1);
        REQUIRE(source2->close_count == 1);
        REQUIRE(source3->close_count == 1);
        REQUIRE(source1->GetEmittedEventCount() == 9);   // 3 dropped, 6 emitted
        REQUIRE(source2->GetEmittedEventCount() == 12);  // 3 dropped, 9 emitted
        REQUIRE(source3->GetEmittedEventCount() == 7);   // 3 dropped, 4 emitted
        REQUIRE(app.GetNEventsProcessed() == 19);
    }


    SECTION("All three event sources initialize, run, and finish, using individualized nskip,nevents (nevents-terminated)") {
        source1->event_bound = 9;
        source2->event_bound = 13;
        source3->event_bound = 7;
        source1->SetNSkip(2);
        source1->SetNEvents(4);
        source3->SetNEvents(4);

        app.SetParameterValue("nthreads", 4);
        app.Run(true);

        REQUIRE(app.GetExitCode() == (int) JApplication::ExitCode::Success);
        REQUIRE(source1->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source2->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source3->GetStatus() == JEventSource::Status::Closed);
        REQUIRE(source1->open_count == 1);
        REQUIRE(source2->open_count == 1);
        REQUIRE(source3->open_count == 1);
        REQUIRE(source1->close_count == 1);
        REQUIRE(source2->close_count == 1);
        REQUIRE(source3->close_count == 1);
        REQUIRE(source1->GetEmittedEventCount() == 6);   // 2 dropped, 4 emitted
        REQUIRE(source2->GetEmittedEventCount() == 13);  // 13 emitted
        REQUIRE(source3->GetEmittedEventCount() == 4);   // 4 emitted
        REQUIRE(app.GetNEventsProcessed() == 21);
    }

}

