
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include "catch.hpp"

int global_resource = 0;


struct BarrierSource : public JEventSource {

    BarrierSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Open() override {
    }

    Result Emit(JEvent& event) override {

        auto event_nr = GetEmittedEventCount() + 1;
        event.SetEventNumber(event_nr);

        if (event_nr % 10 == 0) {
            LOG_INFO(GetLogger()) << "Emitting barrier event " << event_nr << LOG_END;
            event.SetSequential(true);
        }
        else {
            LOG_INFO(GetLogger()) << "Emitting non-barrier event " << event_nr << LOG_END;
        }
        return Result::Success;
    }
};



struct BarrierProcessor : public JEventProcessor {

    BarrierProcessor() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void Process(const JEvent& event) override {

        if (event.GetSequential()) {
            LOG_INFO(GetLogger()) << "Processing barrier event = " << event.GetEventNumber() << ", writing global var = " << global_resource+1 << LOG_END;
            REQUIRE(global_resource == ((event.GetEventNumber() - 1) / 10));
            global_resource += 1;
        }
        else {
            LOG_INFO(GetLogger()) << "Processing non-barrier event = " << event.GetEventNumber() << ", reading global var = " << global_resource << LOG_END;
            REQUIRE(global_resource == (event.GetEventNumber() / 10));
        }
    }
};


TEST_CASE("BarrierEventTests") {
	SECTION("Basic Barrier") {
		JApplication app;
		app.Add(new BarrierProcessor);
		app.Add(new BarrierSource);
		app.SetParameterValue("nthreads", 4);
		app.SetParameterValue("jana:nevents", 40);
		app.Run(true);
	}
};


