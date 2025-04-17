
#include "catch.hpp"

#include <JANA/JEventSource.h>

struct MyEventSource : public JEventSource {
    int open_count = 0;
    int emit_count = 0;
    int close_count = 0;
    int finish_event_count = 0;
    size_t events_in_file = 5;

    MyEventSource() {
        EnableFinishEvent();
    }

    void Open() override {
        REQUIRE(GetApplication() != nullptr);
        LOG_INFO(GetLogger()) << "Open() called" << LOG_END;
        open_count++;
    }
    Result Emit(JEvent&) override {
        emit_count++;
        REQUIRE(GetApplication() != nullptr);
        if (emit_count > events_in_file) {
            LOG_INFO(GetLogger()) << "Emit() called, iteration " << emit_count << ", returning FailureFinished" << LOG_END;
            return Result::FailureFinished;
        }
        LOG_INFO(GetLogger()) << "Emit() called, iteration " << emit_count << ", returning Success" << LOG_END;
        return Result::Success;
    }
    void Close() override {
        REQUIRE(GetApplication() != nullptr);
        LOG_INFO(GetLogger()) << "Close() called" << LOG_END;
        close_count++;
    }
    void FinishEvent(JEvent& event) override {
        LOG_INFO(GetLogger()) << "FinishEvent() called" << LOG_END;
        finish_event_count++;
    }
};

TEST_CASE("JEventSource_ExpertMode_EmitCount") {

    auto sut = new MyEventSource;
    sut->SetCallbackStyle(MyEventSource::CallbackStyle::ExpertMode);
    sut->SetTypeName("MyEventSource");

    JApplication app;
    app.SetParameterValue("jana:loglevel", "off");
    app.Add(sut);

    SECTION("ShutsSelfOff") {
        LOG << "Running test: JEventSource_ExpertMode_EmitCount :: ShutsSelfOff" << LOG_END;
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 6);       // Emit called 5 times successfully and fails on the 6th
        REQUIRE(sut->GetEmittedEventCount() == 5);  // Emits 5 events successfully
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 5);  // All emitted events were finished
        REQUIRE(sut->GetFinishedEventCount() == 5); // This is reflected in the finished count
    }

    SECTION("LimitedByNEvents") {
        LOG << "Running test: JEventSource_ExpertMode_EmitCount :: LimitedByNEvents" << LOG_END;
        app.SetParameterValue("jana:nevents", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 3);        // Emit called 3 times successfully
        REQUIRE(sut->GetEmittedEventCount() == 3);   // Nevents limit discovered outside Emit
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 3);
    }

    SECTION("LimitedByNSkip") {
        LOG << "Running test: JEventSource_ExpertMode_EmitCount :: LimitedByNSkip" << LOG_END;
        app.SetParameterValue("jana:nskip", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 6);        // Emit called 5 times successfully and fails on the 6th
        REQUIRE(sut->GetEmittedEventCount() == 5);   // 5 events successfully emitted, 3 of which were (presumably) skipped
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 2);        // TODO: Should be 5 because every event that is Emit()ted must be FinishEvent()ted
        REQUIRE(sut->GetFinishedEventCount() == 2);   // Only two events were successfully processed, the rest were skipped
        REQUIRE(sut->GetSkippedEventCount() == 3);    // First 3 events skipped out of 5 total
    }
}


