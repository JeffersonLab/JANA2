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


TEST_CASE("greenfield:ActivableTests") {

    LinearTopologyBuilder builder;

    RandIntSource source;
    SumSink<double> sink;

    builder.addSource("emit_rand_ints", source);
    builder.addProcessor<MultByTwoProcessor>("multiply_by_two");
    builder.addProcessor<SubOneProcessor>("subtract_one");
    builder.addSink("sum_everything", sink);

    auto topology = builder.get();

    auto logger = JLogger::nothing(); //everything();
    topology.logger = logger;
    source.logger = logger;


    SECTION("At first, everything is deactivated and all queues are empty") {

        for (auto status : topology.get_queue_status()) {
            REQUIRE(status.is_finished == 1);
            REQUIRE(status.message_count == 0);
        }
        for (auto status : topology.get_arrow_status()) {
            REQUIRE(status.is_finished == 1);
        }
    }
    SECTION("As a message propagates, arrows and queues downstream automatically activate") {

        topology.activate("emit_rand_ints");

        // Activation worked
        auto arrow_statuses = topology.get_arrow_status();
        REQUIRE(arrow_statuses[0].is_finished == false);
        REQUIRE(arrow_statuses[1].is_finished == true);
        REQUIRE(arrow_statuses[2].is_finished == true);
        REQUIRE(arrow_statuses[3].is_finished == true);

        topology.step("emit_rand_ints");

        // Pushing to the queue caused the next arrow to activate
        arrow_statuses = topology.get_arrow_status();
        REQUIRE(arrow_statuses[0].is_finished == false);
        REQUIRE(arrow_statuses[1].is_finished == false);
        REQUIRE(arrow_statuses[2].is_finished == true);
        REQUIRE(arrow_statuses[3].is_finished == true);

        auto queue_statuses = topology.get_queue_status();
        REQUIRE(queue_statuses[0].is_finished == false);
        REQUIRE(queue_statuses[1].is_finished == true);
        REQUIRE(queue_statuses[2].is_finished == true);

        topology.step("multiply_by_two");

        arrow_statuses = topology.get_arrow_status();
        REQUIRE(arrow_statuses[0].is_finished == false);
        REQUIRE(arrow_statuses[1].is_finished == false);
        REQUIRE(arrow_statuses[2].is_finished == false);
        REQUIRE(arrow_statuses[3].is_finished == true);

        queue_statuses = topology.get_queue_status();
        REQUIRE(queue_statuses[0].is_finished == false);
        REQUIRE(queue_statuses[1].is_finished == false);
        REQUIRE(queue_statuses[2].is_finished == true);

    }
} // TEST_CASE
} // namespace greenfield
