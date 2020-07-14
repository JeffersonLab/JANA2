
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>


// This is where you can assemble various components and verify that when put together, they
// do what you'd expect. This means you can skip the laborious mixing and matching of plugins and configurations,
// and have everything run automatically inside one executable.

TEST_CASE("IntegrationTests") {

    auto app = new JApplication;
    
    // Create and register components
    // app->Add(new janacontrolProcessor);

    // Set test parameters
    app->SetParameterValue("nevents", 10);

    // Run everything, blocking until finished
    app->Run();

    // Verify the results you'd expect
    REQUIRE(app->GetNEventsProcessed() == 10);

}

