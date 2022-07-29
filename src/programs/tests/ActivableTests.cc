
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"

#include <TestTopologyComponents.h>
#include <JANA/Engine/JArrowTopology.h>

JArrowMetrics::Status steppe(JArrow* arrow) {
    if (arrow->get_type() != JArrow::NodeType::Source &&
        arrow->get_state() == JArrow::State::Running &&
        arrow->get_pending() == 0 &&
        arrow->get_running_upstreams() == 0) {

        arrow->finish();
        return JArrowMetrics::Status::Finished;
    }

    JArrowMetrics metrics;
        arrow->execute(metrics, 0);
        return metrics.get_last_status();
}

TEST_CASE("ActivableActivationTests") {

    RandIntSource source;
    MultByTwoProcessor p1;
    SubOneProcessor p2;
    SumSink<double> sink;

    JArrowTopology topology;

    auto q1 = new JMailbox<int>();
    auto q2 = new JMailbox<double>();
    auto q3 = new JMailbox<double>();

    auto emit_rand_ints = new SourceArrow<int>("emit_rand_ints", source, q1);
    auto multiply_by_two = new MapArrow<int,double>("multiply_by_two", p1, q1, q2);
    auto subtract_one = new MapArrow<double,double>("subtract_one", p2, q2, q3);
    auto sum_everything = new SinkArrow<double>("sum_everything", sink, q3);

    emit_rand_ints->attach(multiply_by_two);
    multiply_by_two->attach(subtract_one);
    subtract_one->attach(sum_everything);

    topology.sources.push_back(emit_rand_ints);

    topology.arrows.push_back(emit_rand_ints);
    topology.arrows.push_back(multiply_by_two);
    topology.arrows.push_back(subtract_one);
    topology.arrows.push_back(sum_everything);
    topology.sinks.push_back(sum_everything);

    emit_rand_ints->set_chunksize(1);

    auto logger = JLogger(JLogger::Level::OFF);
    topology.m_logger = logger;
    source.logger = logger;


    SECTION("At first, everything is deactivated and all queues are empty") {

        REQUIRE(q1->size() == 0);
        REQUIRE(q2->size() == 0);
        REQUIRE(q3->size() == 0);

        REQUIRE(emit_rand_ints->get_state() == JArrow::State::Unopened);
        REQUIRE(multiply_by_two->get_state() == JArrow::State::Unopened);
        REQUIRE(subtract_one->get_state() == JArrow::State::Unopened);
        REQUIRE(sum_everything->get_state() == JArrow::State::Unopened);

        REQUIRE(emit_rand_ints->get_pending() == 0);
        REQUIRE(multiply_by_two->get_pending() == 0);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);
    }

    SECTION("As a message propagates, arrows and queues downstream automatically activate") {

        REQUIRE(emit_rand_ints->get_state() == JArrow::State::Unopened);
        REQUIRE(multiply_by_two->get_state() == JArrow::State::Unopened);
        REQUIRE(subtract_one->get_state() == JArrow::State::Unopened);
        REQUIRE(sum_everything->get_state() == JArrow::State::Unopened);

        topology.run();
        // TODO: Check that initialize has been called, but not finalize

        REQUIRE(emit_rand_ints->get_state() == JArrow::State::Running);
        REQUIRE(multiply_by_two->get_state() == JArrow::State::Running);
        REQUIRE(subtract_one->get_state() == JArrow::State::Running);
        REQUIRE(sum_everything->get_state() == JArrow::State::Running);
    }

} // TEST_CASE


TEST_CASE("ActivableDeactivationTests") {

    RandIntSource source;
    source.emit_limit = 1;

    MultByTwoProcessor p1;
    SubOneProcessor p2;
    SumSink<double> sink;

    JArrowTopology topology;

    auto q1 = new JMailbox<int>();
    auto q2 = new JMailbox<double>();
    auto q3 = new JMailbox<double>();

    auto emit_rand_ints = new SourceArrow<int>("emit_rand_ints", source, q1);
    auto multiply_by_two = new MapArrow<int,double>("multiply_by_two", p1, q1, q2);
    auto subtract_one = new MapArrow<double,double>("subtract_one", p2, q2, q3);
    auto sum_everything = new SinkArrow<double>("sum_everything", sink, q3);

    emit_rand_ints->attach(multiply_by_two);
    multiply_by_two->attach(subtract_one);
    subtract_one->attach(sum_everything);

    topology.sources.push_back(emit_rand_ints);
    topology.arrows.push_back(emit_rand_ints);
    topology.arrows.push_back(multiply_by_two);
    topology.arrows.push_back(subtract_one);
    topology.arrows.push_back(sum_everything);
    topology.sinks.push_back(sum_everything);

    auto logger = JLogger(JLogger::Level::OFF);
    topology.m_logger = logger;
    source.logger = logger;

    REQUIRE(emit_rand_ints->get_state() == JArrow::State::Unopened);
    REQUIRE(multiply_by_two->get_state() == JArrow::State::Unopened);
    REQUIRE(subtract_one->get_state() == JArrow::State::Unopened);
    REQUIRE(sum_everything->get_state() == JArrow::State::Unopened);

    topology.run();

    REQUIRE(emit_rand_ints->get_state() == JArrow::State::Running);
    REQUIRE(multiply_by_two->get_state() == JArrow::State::Running);
    REQUIRE(subtract_one->get_state() == JArrow::State::Running);
    REQUIRE(sum_everything->get_state() == JArrow::State::Running);

    steppe(emit_rand_ints);

    REQUIRE(emit_rand_ints->get_state() == JArrow::State::Finished);
    REQUIRE(multiply_by_two->get_state() == JArrow::State::Running);
    REQUIRE(subtract_one->get_state() == JArrow::State::Running);
    REQUIRE(sum_everything->get_state() == JArrow::State::Running);

    // TODO: Test that finalize was called exactly once
} // TEST_CASE








