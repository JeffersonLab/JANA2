
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"

#include "../Topology/TestTopologyComponents.h"
#include <JANA/Engine/JScheduler.h>


JArrowMetrics::Status steppe(JArrow* arrow) {
    JArrowMetrics metrics;
    arrow->execute(metrics, 0);
    return metrics.get_last_status();
}

TEST_CASE("ArrowActivationTests") {

    JApplication app;
    app.Initialize();
    auto jcm = app.GetService<JComponentManager>();

    auto q1 = new JMailbox<EventT*>();
    auto q2 = new JMailbox<EventT*>();
    auto q3 = new JMailbox<EventT*>();

    auto p1 = new JEventPool(jcm, 0,1,false);
    auto p2 = new JEventPool(jcm, 0,1,false);
    p1->init();
    p2->init();

    auto emit_rand_ints = new RandIntArrow("emit_rand_ints", p1, q1);
    auto multiply_by_two = new MultByTwoArrow("multiply_by_two", q1, q2);
    auto subtract_one = new SubOneArrow("subtract_one", q2, q3);
    auto sum_everything = new SumArrow("sum_everything", q3, p2);

    auto topology = std::make_shared<JTopologyBuilder>();

    emit_rand_ints->attach(multiply_by_two);
    multiply_by_two->attach(subtract_one);
    subtract_one->attach(sum_everything);

    topology->arrows.push_back(emit_rand_ints);
    topology->arrows.push_back(multiply_by_two);
    topology->arrows.push_back(subtract_one);
    topology->arrows.push_back(sum_everything);

    topology->pools.push_back(p1);
    topology->pools.push_back(p2);

    auto logger = JLogger(JLogger::Level::OFF);
    topology->SetLogger(logger);
    emit_rand_ints->set_logger(logger);
    multiply_by_two->set_logger(logger);
    subtract_one->set_logger(logger);
    sum_everything->set_logger(logger);

    JScheduler scheduler(topology);
    scheduler.logger = logger;


    SECTION("At first, everything is deactivated and all queues are empty") {

        REQUIRE(q1->size() == 0);
        REQUIRE(q2->size() == 0);
        REQUIRE(q3->size() == 0);

        JScheduler::TopologyState state = scheduler.get_topology_state();

        REQUIRE(state.arrow_states[0].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[1].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[2].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[3].status == JScheduler::ArrowStatus::Uninitialized);

        REQUIRE(emit_rand_ints->get_pending() == 0);
        REQUIRE(multiply_by_two->get_pending() == 0);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);
    }

    SECTION("As a message propagates, arrows and queues downstream automatically activate") {

        JScheduler::TopologyState state = scheduler.get_topology_state();

        REQUIRE(state.arrow_states[0].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[1].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[2].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[3].status == JScheduler::ArrowStatus::Uninitialized);

        scheduler.run_topology(1);
        state = scheduler.get_topology_state();

        REQUIRE(state.arrow_states[0].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[1].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[2].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[3].status == JScheduler::ArrowStatus::Active);
    }

    SECTION("Deactivation") {

        emit_rand_ints->emit_limit = 1;

        JScheduler::TopologyState state = scheduler.get_topology_state();

        REQUIRE(state.arrow_states[0].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[1].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[2].status == JScheduler::ArrowStatus::Uninitialized);
        REQUIRE(state.arrow_states[3].status == JScheduler::ArrowStatus::Uninitialized);

        scheduler.run_topology(1);
        state = scheduler.get_topology_state();

        REQUIRE(state.arrow_states[0].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[1].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[2].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[3].status == JScheduler::ArrowStatus::Active);

        auto result = steppe(emit_rand_ints);
        REQUIRE(result == JArrowMetrics::Status::Finished);

        scheduler.next_assignment(0, emit_rand_ints, result);
        state = scheduler.get_topology_state();

        REQUIRE(state.arrow_states[0].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(state.arrow_states[1].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[2].status == JScheduler::ArrowStatus::Active);
        REQUIRE(state.arrow_states[3].status == JScheduler::ArrowStatus::Active);

    }

} // TEST_CASE







