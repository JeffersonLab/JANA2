
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "BarrierEventTests.h"
#include "catch.hpp"

TEST_CASE("BarrierEventTests") {
	SECTION("Basic Barrier") {
		JApplication app;
		app.Add(new BarrierProcessor(&app));
		app.Add(new BarrierSource("dummy", &app));
		app.SetParameterValue("nthreads", 4);
        app.SetParameterValue("jana:event_source_chunksize", 1);
        app.SetParameterValue("jana:event_processor_chunksize", 1);
		app.Run(true);
	}
};

