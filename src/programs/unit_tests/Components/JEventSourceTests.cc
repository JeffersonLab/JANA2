
#include "catch.hpp"

#include <JANA/JEventSource.h>

struct MyEventSource : public JEventSource {
    int open_count = 0;
    int emit_count = 0;
    int close_count = 0;
    size_t events_in_file = 5;

    void Open() override {
        REQUIRE(GetApplication() != nullptr);
        LOG_INFO(GetLogger()) << "Open() called" << LOG_END;
        open_count++;
    }
    Result Emit(JEvent&) override {
        emit_count++;
        REQUIRE(GetApplication() != nullptr);
        if (GetEmittedEventCount() >= events_in_file) {
            LOG_INFO(GetLogger()) << "Emit() called, returning FailureFinished" << LOG_END;
            return Result::FailureFinished;
        }
        LOG_INFO(GetLogger()) << "Emit() called, returning Success" << LOG_END;
        return Result::Success;
    }
    void Close() override {
        REQUIRE(GetApplication() != nullptr);
        LOG_INFO(GetLogger()) << "Close() called" << LOG_END;
        close_count++;
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
        REQUIRE(sut->GetEmittedEventCount() == 5);  // Emits 5 events successfully (including skipped)
        REQUIRE(sut->close_count == 1);
    }

    SECTION("LimitedByNEvents") {
        LOG << "Running test: JEventSource_ExpertMode_EmitCount :: LimitedByNEvents" << LOG_END;
        app.SetParameterValue("jana:nevents", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 3);        // Emit called 3 times successfully
        REQUIRE(sut->GetEmittedEventCount() == 3);   // Nevents limit discovered outside Emit
        REQUIRE(sut->close_count == 1);
    }

    SECTION("LimitedByNSkip") {
        LOG << "Running test: JEventSource_ExpertMode_EmitCount :: LimitedByNSkip" << LOG_END;
        app.SetParameterValue("jana:nskip", 3);
        app.Run();
        REQUIRE(sut->open_count == 1);
        REQUIRE(sut->emit_count == 6);        // Emit called 5 times successfully and fails on the 6th
        REQUIRE(sut->GetEmittedEventCount() == 5);   // 5 events successfully emitted, 3 of which were (presumably) skipped
        REQUIRE(sut->close_count == 1);
    }
}


