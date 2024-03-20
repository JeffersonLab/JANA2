
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"
#include "ExactlyOnceTests.h"

#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JApplication.h>

TEST_CASE("ExactlyOnceTests") {

    JApplication app;

    auto source = new SimpleSource();
    auto processor = new SimpleProcessor();

    app.Add(source);
    app.Add(processor);
    app.SetParameterValue("jana:extended_report", 0);

    REQUIRE(source->open_count == 0);
    REQUIRE(source->close_count == 0);
    REQUIRE(processor->init_count == 0);
    REQUIRE(processor->finish_count == 0);

    app.Run(true);

    REQUIRE(source->open_count == 1);
    REQUIRE(source->close_count == 1);
    REQUIRE(processor->init_count == 1);
    REQUIRE(processor->finish_count == 1);
}



