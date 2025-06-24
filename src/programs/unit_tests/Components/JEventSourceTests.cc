
#include "catch.hpp"

#include <JANA/JEventSource.h>

struct MyEventSource : public JEventSource {
    int open_count = 0;
    int emit_count = 0;
    int close_count = 0;
    int finish_event_count = 0;
    size_t events_in_file = 5;
    int events_per_barrier = 0;

    MyEventSource() {
        SetTypeName("MyEventSource");
        EnableFinishEvent();
    }

    void Open() override {
        REQUIRE(GetApplication() != nullptr);
        LOG_INFO(GetLogger()) << "Open() called" << LOG_END;
        open_count++;
    }
    Result Emit(JEvent& event) override {
        emit_count++;
        if ((events_per_barrier != 0) && ((emit_count % events_per_barrier) == 0)) {
            event.SetSequential(true);
        }
        REQUIRE(GetApplication() != nullptr);
        if (emit_count > (int) events_in_file) {
            LOG_INFO(GetLogger()) << "Emit() called, iteration " << emit_count << ", returning FailureFinished" << LOG_END;
            return Result::FailureFinished;
        }
        LOG_INFO(GetLogger()) << "Emit() called, iteration " << emit_count << ", returning Success" << LOG_END;
        return Result::Success;
    }
    void GetEvent(std::shared_ptr<JEvent> event) override {
        emit_count++;
        if ((events_per_barrier != 0) && ((emit_count % events_per_barrier) == 0)) {
            event->SetSequential(true);
        }
        REQUIRE(GetApplication() != nullptr);
        if (emit_count > (int) events_in_file) {
            LOG_INFO(GetLogger()) << "GetEvent() called, iteration " << emit_count << ", returning FailureFinished" << LOG_END;
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
        LOG_INFO(GetLogger()) << "GetEvent() called, iteration " << emit_count << ", returning Success" << LOG_END;
    }
    void Close() override {
        REQUIRE(GetApplication() != nullptr);
        LOG_INFO(GetLogger()) << "Close() called" << LOG_END;
        close_count++;
    }
    void FinishEvent(JEvent&) override {
        LOG_INFO(GetLogger()) << "FinishEvent() called" << LOG_END;
        finish_event_count++;
    }
};

TEST_CASE("JEventSource_EmitCount") {

    auto sut = new MyEventSource;
    JApplication app;
    app.SetParameterValue("jana:loglevel", "off");
    app.Add(sut);

    SECTION("ShutsSelfOff_ExpertMode_NoBarriers") {
        LOG << "Running test: JEventSource_EmitCount :: ShutsSelfOff_ExpertMode_NoBarriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::ExpertMode);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 6);       // Emit called 5 times successfully and fails on the 6th
        REQUIRE(sut->GetEmittedEventCount() == 5);  // Emits 5 events successfully
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 5);  // All emitted events were finished
        REQUIRE(sut->GetProcessedEventCount() == 5); // This is reflected in the finished count
    }

    SECTION("LimitedByNEvents_ExpertMode_NoBarriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNEvents_ExpertMode_NoBarriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::ExpertMode);

        app.SetParameterValue("jana:nevents", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 3);        // Emit called 3 times successfully
        REQUIRE(sut->GetEmittedEventCount() == 3);   // Nevents limit discovered outside Emit
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 3);
    }

    SECTION("LimitedByNSkip_ExpertMode_NoBarriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNSkip_ExpertMode_NoBarriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::ExpertMode);

        app.SetParameterValue("jana:nskip", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 6);                // Emit was called 5 times successfully and failed on the 6th
        REQUIRE(sut->GetEmittedEventCount() == 2);    // The first 3 were skipped, so only 2 entered the topology
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 5);        // FinishEvent() was called for both emitted and skipped events
        REQUIRE(sut->GetProcessedEventCount() == 2);  // The 2 emitted events were both successfully processed
        REQUIRE(sut->GetSkippedEventCount() == 3);    // The first 3 events were skipped, out of 5 total
    }

    SECTION("ShutsSelfOff_LegacyMode_NoBarriers") {
        LOG << "Running test: JEventSource_EmitCount :: ShutsSelfOff_LegacyMode_NoBarriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::LegacyMode);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 6);       // Emit called 5 times successfully and fails on the 6th
        REQUIRE(sut->GetEmittedEventCount() == 5);  // Emits 5 events successfully
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 5);  // All emitted events were finished
        REQUIRE(sut->GetProcessedEventCount() == 5); // This is reflected in the finished count
    }

    SECTION("LimitedByNEvents_LegacyMode_NoBarriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNEvents_LegacyMode_NoBarriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::LegacyMode);
        app.SetParameterValue("jana:nevents", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 3);        // Emit called 3 times successfully
        REQUIRE(sut->GetEmittedEventCount() == 3);   // Nevents limit discovered outside Emit
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 3);
    }

    SECTION("LimitedByNSkip_LegacyMode_NoBarriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNSkip_LegacyMode_NoBarriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::LegacyMode);
        app.SetParameterValue("jana:nskip", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 6);                // Emit was called 5 times successfully and failed on the 6th
        REQUIRE(sut->GetEmittedEventCount() == 2);    // The first 3 were skipped, so only 2 entered the topology
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 5);        // FinishEvent() was called for both emitted and skipped events
        REQUIRE(sut->GetProcessedEventCount() == 2);  // The 2 emitted events were both successfully processed
        REQUIRE(sut->GetSkippedEventCount() == 3);    // The first 3 events were skipped, out of 5 total
    }

    SECTION("ShutsSelfOff_ExpertMode_Barriers") {
        LOG << "Running test: JEventSource_EmitCount :: ShutsSelfOff_ExpertMode_Barriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::ExpertMode);
        sut->events_per_barrier = 4;
        sut->events_in_file = 50;
        app.SetParameterValue("nthreads", 8);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 51);               // Emit called 50 times successfully and fails on the 51st
        REQUIRE(sut->GetEmittedEventCount() == 50);   // Emits 50 events successfully
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 50);       // All emitted events were finished
        REQUIRE(sut->GetProcessedEventCount() == 50); // This is reflected in the finished count
    }

    SECTION("LimitedByNEvents_ExpertMode_Barriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNEvents_ExpertMode_Barriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::ExpertMode);
        sut->events_per_barrier = 4;
        sut->events_in_file = 50;
        app.SetParameterValue("nthreads", 8);
        app.SetParameterValue("jana:nevents", 30);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 30);               // Emit called 30 times successfully
        REQUIRE(sut->GetEmittedEventCount() ==  30);  // Nevents limit discovered outside Emit
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 30);
        REQUIRE(sut->GetProcessedEventCount() == 30);
    }

    SECTION("LimitedByNSkip_ExpertMode_Barriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNSkip_ExpertMode_Barriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::ExpertMode);
        sut->events_per_barrier = 4;
        sut->events_in_file = 50;
        app.SetParameterValue("nthreads", 8);
        app.SetParameterValue("jana:nskip", 10);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 51);                // GetEvent was called 50 times successfully and failed on the 51st
        REQUIRE(sut->GetEmittedEventCount() == 40);    // The first 10 were skipped, so only 40 entered the topology
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 50);        // FinishEvent() was called for both emitted and skipped events
        REQUIRE(sut->GetProcessedEventCount() == 40);  // The 40 emitted events were successfully processed
        REQUIRE(sut->GetSkippedEventCount() == 10);    // The first 10 events were skipped, out of 50 total
    }

    SECTION("ShutsSelfOff_LegacyMode_Barriers") {
        LOG << "Running test: JEventSource_EmitCount :: ShutsSelfOff_LegacyMode_Barriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::LegacyMode);
        sut->events_per_barrier = 4;
        sut->events_in_file = 50;
        app.SetParameterValue("nthreads", 8);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 51);               // Emit called 50 times successfully and fails on the 51st
        REQUIRE(sut->GetEmittedEventCount() == 50);   // Emits 50 events successfully
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 50);       // All emitted events were finished
        REQUIRE(sut->GetProcessedEventCount() == 50); // This is reflected in the finished count
    }

    SECTION("LimitedByNEvents_LegacyMode_Barriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNEvents_LegacyMode_Barriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::LegacyMode);
        sut->events_per_barrier = 4;
        sut->events_in_file = 50;
        app.SetParameterValue("nthreads", 8);
        app.SetParameterValue("jana:nevents", 30);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 30);               // Emit called 30 times successfully
        REQUIRE(sut->GetEmittedEventCount() ==  30);  // Nevents limit discovered outside Emit
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 30);
        REQUIRE(sut->GetProcessedEventCount() == 30);
    }

    SECTION("LimitedByNSkip_LegacyMode_Barriers") {
        LOG << "Running test: JEventSource_EmitCount :: LimitedByNSkip_LegacyMode_Barriers" << LOG_END;
        sut->SetCallbackStyle(MyEventSource::CallbackStyle::LegacyMode);
        sut->events_per_barrier = 4;
        sut->events_in_file = 50;
        app.SetParameterValue("nthreads", 8);
        app.SetParameterValue("jana:nskip", 10);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 51);                // GetEvent was called 50 times successfully and failed on the 51st
        REQUIRE(sut->GetEmittedEventCount() == 40);    // The first 10 were skipped, so only 40 entered the topology
        REQUIRE(sut->close_count == 1);
        REQUIRE(sut->finish_event_count == 50);        // FinishEvent() was called for both emitted and skipped events
        REQUIRE(sut->GetProcessedEventCount() == 40);  // The 40 emitted events were successfully processed
        REQUIRE(sut->GetSkippedEventCount() == 10);    // The first 10 events were skipped, out of 50 total
    }
}

