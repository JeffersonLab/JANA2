// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include "../Tutorial/Hit.h"
#include "../Tutorial/SimpleClusterFactory.h"

TEST_CASE("SimpleClusterFactoryTests") {

    // We need to fire up the JApplication so that our Factory can access all of its JServices.
    // However, for unit testing, we don't need (or want!) to set up an event source or actually call JApplication::Run().
    JApplication app;
    app.SetParameterValue("log:debug", "SimpleClusterFactory");
    app.SetParameterValue("log:off", "JApplication,JParameterManager,JArrowProcessingController,JArrow");
    // Add any plugins you need here
    // app.AddPlugin("myPlugin");
    app.Initialize();  // Load the plugins

    // Create an instance of the JFactory we are testing
    // Unlike with regular JANA, we can hold on to a pointer to the factory, and ask it questions directly.
    auto factory = new SimpleClusterFactory;

    // Create a JEvent and give it the factory
    auto event = std::make_shared<JEvent>();
    event->SetJApplication(&app);
    event->GetFactorySet()->Add(factory);

    // We can now set up different independent scenarios and see if they behave according to our expectations.
    // Note that Catch2 will re-run the code above for each SECTION it sees below, so the test cases stay
    // isolated from each other.

    SECTION("One hit => One cluster") {
        // We set up a dummy scenario for each test case, craft some inputs manually, and insert them into the event.
        // If we want, we can add other factories, as long as we insert _their_ inputs as well.
        // However, the fewer moving pieces in the test, the better. So right now our event only knows about the
        // factory being tested, and all of its inputs are just inserted data.
        std::vector<Hit*> hits;
        hits.push_back(new Hit(5,5,1.0,0.0));
        event->Insert(hits);

        auto clusters = event->Get<Cluster>();
        // This is the best way to call the factory being tested because it makes sure Initialize(), BeginRun(), etc
        // have been called as needed.

        // Finally we make some assertions about our output data.
        REQUIRE(clusters.size() == 1);
    }

    SECTION("Cluster is centered on hits") {
        std::vector<Hit*> hits;
        hits.push_back(new Hit(0,10,1.0,0.0));
        hits.push_back(new Hit(6,12,1.0,0.0));
        event->Insert(hits);

        auto clusters = event->Get<Cluster>(); // This ultimately calls your factory
        REQUIRE(clusters.size() == 1);
        REQUIRE(clusters[0]->x_center == 3);
        REQUIRE(clusters[0]->y_center == 11);
    }

    SECTION("Every hit is part of at least one cluster") {
        // This brings up the idea of property-based testing. This would be more
        // like an integration test. The idea is that you create a set of test cases that
        // each test some property that you expect to hold no matter what the inputs. Instead of
        // hand-crafting test inputs for each property, you just put the property assertions in one JEventProcessor.
        // This way, you can add the JEventProcessor at any time, and it will verify that the property holds
        // for whatever event source and factory plugins you provide.
        REQUIRE(true == true);
    }

}
