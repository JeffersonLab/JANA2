//
// Created by nbrei on 3/26/19.
//


#include "catch.hpp"

#include <thread>
#include <random>
#include <greenfield/Topology.h>
#include <greenfield/LinearTopologyBuilder.h>
#include "greenfield/ExampleComponents.h"


namespace greenfield {


TEST_CASE("greenfield:ActivableActivationTests") {

    LinearTopologyBuilder builder;

    RandIntSource source;
    SumSink<double> sink;

    builder.addSource("emit_rand_ints", source);
    builder.addProcessor<MultByTwoProcessor>("multiply_by_two");
    builder.addProcessor<SubOneProcessor>("subtract_one");
    builder.addSink("sum_everything", sink);

    auto topology = builder.get();

    auto logger = Logger::nothing(); //everything();
    topology.logger = logger;
    source.logger = logger;


    SECTION("At first, everything is deactivated and all queues are empty") {

        for (auto status : topology.get_queue_status()) {
            REQUIRE(status.is_active == false);
            REQUIRE(status.message_count == 0);
        }
        for (auto status : topology.get_arrow_status()) {
            REQUIRE(status.is_active == false);
        }
    }
    SECTION("As a message propagates, arrows and queues downstream automatically activate") {

        auto arrow_statuses = topology.get_arrow_status();
        REQUIRE(arrow_statuses[0].is_active == false);
        REQUIRE(arrow_statuses[1].is_active == false);
        REQUIRE(arrow_statuses[2].is_active == false);
        REQUIRE(arrow_statuses[3].is_active == false);

        topology.activate("emit_rand_ints");

        // Activation worked
        arrow_statuses = topology.get_arrow_status();
        REQUIRE(arrow_statuses[0].is_active == true);
        REQUIRE(arrow_statuses[1].is_active == true);
        REQUIRE(arrow_statuses[2].is_active == true);
        REQUIRE(arrow_statuses[3].is_active == true);

    }
} // TEST_CASE


TEST_CASE("greenfield:ActivableDeactivationTests") {

    LinearTopologyBuilder builder;

    RandIntSource source;
    source.emit_limit = 1;
    SumSink<double> sink;

    builder.addSource("a", source);
    builder.addProcessor<MultByTwoProcessor>("b");
    builder.addProcessor<SubOneProcessor>("c");
    builder.addSink("d", sink);

    auto topology = builder.get();
    auto logger = Logger::nothing();
    topology.logger = logger;
    source.logger = logger;

    REQUIRE(topology.get_status("a").is_active == false);
    REQUIRE(topology.get_status("b").is_active == false);
    REQUIRE(topology.get_status("c").is_active == false);
    REQUIRE(topology.get_status("d").is_active == false);

    topology.activate("a");

    REQUIRE(topology.get_status("a").is_active == true);
    REQUIRE(topology.get_status("b").is_active == true);
    REQUIRE(topology.get_status("c").is_active == true);
    REQUIRE(topology.get_status("d").is_active == true);

    topology.step("a");

    REQUIRE(topology.get_status("a").is_active == false);
    REQUIRE(topology.get_status("b").is_active == true);
    REQUIRE(topology.get_status("c").is_active == true);
    REQUIRE(topology.get_status("d").is_active == true);

    topology.log_status();


} // TEST_CASE
} // namespace greenfield








