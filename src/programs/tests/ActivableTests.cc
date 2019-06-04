//
// Created by nbrei on 3/26/19.
//


#include "catch.hpp"

#include <thread>
#include <random>
#include <TestTopology.h>
#include <TestTopologyComponents.h>


TEST_CASE("ActivableActivationTests") {

    RandIntSource source;
    MultByTwoProcessor p1;
    SubOneProcessor p2;
    SumSink<double> sink;

    TestTopology topology;

    auto q1 = new JMailbox<int>();
    auto q2 = new JMailbox<double>();
    auto q3 = new JMailbox<double>();

    topology.addArrow(new SourceArrow<int>("emit_rand_ints", source, q1));
    topology.addArrow(new MapArrow<int,double>("multiply_by_two", p1, q1, q2));
    topology.addArrow(new MapArrow<double,double>("subtract_one", p2, q2, q3));
    topology.addArrow(new SinkArrow<double>("sum_everything", sink, q3));

    topology.get_arrow("emit_rand_ints")->set_chunksize(1);

    auto logger = JLogger::nothing(); //everything();
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


TEST_CASE("ActivableDeactivationTests") {

    RandIntSource source;
    source.emit_limit = 1;

    MultByTwoProcessor p1;
    SubOneProcessor p2;
    SumSink<double> sink;

    TestTopology topology;

    auto q1 = new JMailbox<int>();
    auto q2 = new JMailbox<double>();
    auto q3 = new JMailbox<double>();

    topology.addArrow(new SourceArrow<int>("a", source, q1));
    topology.addArrow(new MapArrow<int,double>("b", p1, q1, q2));
    topology.addArrow(new MapArrow<double,double>("c", p2, q2, q3));
    topology.addArrow(new SinkArrow<double>("d", sink, q3));

    auto logger = JLogger::nothing();
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








