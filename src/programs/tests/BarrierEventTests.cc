
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "BarrierEventTests.h"
#include "catch.hpp"

TEST_CASE("BarrierEventTests") {
	SECTION("Basic Barrier") {
		JApplication app;
		app.Add(new BarrierProcessor(&app));
		app.Add(new BarrierSource("dummy", &app));
		app.Run(true);
	}
};

