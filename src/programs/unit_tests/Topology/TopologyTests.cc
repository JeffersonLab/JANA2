
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include "JANA/Topology/JTopologyBuilder.h"
#include <JANA/Engine/JArrowProcessingController.h>

#include "TestTopologyComponents.h"
#include <JANA/Utils/JPerfUtils.h>
#include <JANA/Engine/JScheduler.h>


JArrowMetrics::Status step(JArrow* arrow) {

        JArrowMetrics metrics;
        arrow->execute(metrics, 0);
        auto status = metrics.get_last_status();
        return status;
}




TEST_CASE("JTopology: Basic functionality") {

    auto q1 = new JMailbox<int*>();
    auto q2 = new JMailbox<double*>();
    auto q3 = new JMailbox<double*>();

    auto p1 = new JPool<int>(0,1,false);
    auto p2 = new JPool<double>(0,1,false);
    p1->init();
    p2->init();

    MultByTwoProcessor processor;

    auto emit_rand_ints = new RandIntSource("emit_rand_ints", p1, q1);
    auto multiply_by_two = new MapArrow<int*,double*>("multiply_by_two", processor, q1, q2);
    auto subtract_one = new SubOneProcessor("subtract_one", q2, q3);
    auto sum_everything = new SumSink<double>("sum_everything", q3, p2);

    auto topology = std::make_shared<JTopologyBuilder>();

    emit_rand_ints->attach(multiply_by_two);
    multiply_by_two->attach(subtract_one);
    subtract_one->attach(sum_everything);

    topology->arrows.push_back(emit_rand_ints);
    topology->arrows.push_back(multiply_by_two);
    topology->arrows.push_back(subtract_one);
    topology->arrows.push_back(sum_everything);

    auto logger = JLogger(JLogger::Level::INFO);
    topology->SetLogger(logger);
    emit_rand_ints->set_logger(logger);
    multiply_by_two->set_logger(logger);
    subtract_one->set_logger(logger);
    sum_everything->set_logger(logger);

    emit_rand_ints->set_chunksize(1);

    JScheduler scheduler(topology);
    scheduler.logger = logger;

    SECTION("Before anything runs...") {

        // All queues are empty, none are finished
        REQUIRE(multiply_by_two->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);
        REQUIRE(subtract_one->get_pending() == 0);
    }

    SECTION("When nothing is in the input queue...") {

        LOG_INFO(logger) << "Nothing has run yet; should be empty" << LOG_END;

        //topology.log_queue_status();
        step(multiply_by_two);
        step(subtract_one);
        step(sum_everything);
        step(multiply_by_two);
        step(subtract_one);
        step(sum_everything);
        //topology.log_queue_status();

        // All `execute` operations are no-ops
        REQUIRE(multiply_by_two->get_pending() == 0);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);
    }

    SECTION("After emitting") {
        LOG_INFO(logger) << "After emitting; should be something in q0" << LOG_END;

        scheduler.run_topology(1);
        step(emit_rand_ints);

        REQUIRE(multiply_by_two->get_pending() == 1);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);

        step(emit_rand_ints);
        step(emit_rand_ints);
        step(emit_rand_ints);
        step(emit_rand_ints);

        REQUIRE(multiply_by_two->get_pending() == 5);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);

    }

    SECTION("Running each stage sequentially yields the correct results") {

        //LOG_INFO(logger) << "Running each stage sequentially yields the correct results" << LOG_END;
        scheduler.run_topology(1);

        for (int i = 0; i < 20; ++i) {
            step(emit_rand_ints);
            REQUIRE(multiply_by_two->get_pending() == 1);
            REQUIRE(subtract_one->get_pending() == 0);
            REQUIRE(sum_everything->get_pending() == 0);

            step(multiply_by_two);
            REQUIRE(multiply_by_two->get_pending() == 0);
            REQUIRE(subtract_one->get_pending() == 1);
            REQUIRE(sum_everything->get_pending() == 0);

            step(subtract_one);
            REQUIRE(multiply_by_two->get_pending() == 0);
            REQUIRE(subtract_one->get_pending() == 0);
            REQUIRE(sum_everything->get_pending() == 1);

            step(sum_everything);
            REQUIRE(multiply_by_two->get_pending() == 0);
            REQUIRE(subtract_one->get_pending() == 0);
            REQUIRE(sum_everything->get_pending() == 0);
        }

        REQUIRE(sum_everything->sum == (7 * 2.0 - 1) * 20);
    }

    SECTION("Running each stage in random order (sequentially) yields the correct results") {
        //LOG_INFO(logger) << "Running each stage in arbitrary order yields the correct results" << LOG_END;

        JArrow* arrows[] = {emit_rand_ints,
                            multiply_by_two,
                            subtract_one,
                            sum_everything};

        std::map<std::string, JArrowMetrics::Status> results;
        results["emit_rand_ints"] = JArrowMetrics::Status::KeepGoing;
        results["multiply_by_two"] = JArrowMetrics::Status::KeepGoing;
        results["subtract_one"] = JArrowMetrics::Status::KeepGoing;
        results["sum_everything"] = JArrowMetrics::Status::KeepGoing;

        // Put something in the queue to get started
        scheduler.run_topology(1);
        step(emit_rand_ints);

        bool work_left = true;
        while (work_left) {
            // Pick a random arrow
            JArrow* arrow = arrows[randint(0, 3)];

            auto name = arrow->get_name();
            JArrowMetrics metrics;
            arrow->execute(metrics, 0);
            auto res = metrics.get_last_status();
            results[name] = res;
            //LOG_TRACE(logger) << name << " => "
            //                  << to_string(res) << LOG_END;

            size_t pending = multiply_by_two->get_pending() +
                             subtract_one->get_pending() +
                             sum_everything->get_pending();

            work_left = (pending > 0);

            for (auto pair : results) {
                if (pair.second == JArrowMetrics::Status::KeepGoing) { work_left = true; }
            }
        }

        //topology.log_queue_status();
        REQUIRE(sum_everything->sum == (7 * 2.0 - 1) * 20);
    }
    SECTION("Finished flag propagates") {

        scheduler.run_topology(1);
        auto ts = scheduler.get_topology_state();
        JArrowMetrics::Status status;

        REQUIRE(ts.arrow_states[0].status == JScheduler::ArrowStatus::Active);
        REQUIRE(ts.arrow_states[1].status == JScheduler::ArrowStatus::Active);
        REQUIRE(ts.arrow_states[2].status == JScheduler::ArrowStatus::Active);
        REQUIRE(ts.arrow_states[3].status == JScheduler::ArrowStatus::Active);

        for (int i = 0; i < 20; ++i) {
            status = step(emit_rand_ints);
        }
        scheduler.next_assignment(0, emit_rand_ints, status);
        ts = scheduler.get_topology_state();

        REQUIRE(ts.arrow_states[0].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[1].status == JScheduler::ArrowStatus::Active);
        REQUIRE(ts.arrow_states[2].status == JScheduler::ArrowStatus::Active);
        REQUIRE(ts.arrow_states[3].status == JScheduler::ArrowStatus::Active);

        for (int i = 0; i < 20; ++i) {
            step(multiply_by_two);
        }
        scheduler.next_assignment(0, multiply_by_two, status);
        ts = scheduler.get_topology_state();

        REQUIRE(ts.arrow_states[0].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[1].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[2].status == JScheduler::ArrowStatus::Active);
        REQUIRE(ts.arrow_states[3].status == JScheduler::ArrowStatus::Active);

        for (int i = 0; i < 20; ++i) {
            step(subtract_one);
        }
        scheduler.next_assignment(0, subtract_one, status);
        ts = scheduler.get_topology_state();

        REQUIRE(ts.arrow_states[0].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[1].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[2].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[3].status == JScheduler::ArrowStatus::Active);

        for (int i = 0; i < 20; ++i) {
            step(sum_everything);
        }
        scheduler.next_assignment(0, sum_everything, status);
        ts = scheduler.get_topology_state();

        REQUIRE(ts.arrow_states[0].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[1].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[2].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(ts.arrow_states[3].status == JScheduler::ArrowStatus::Finalized);

    }

    SECTION("Running from inside JApplication returns the correct answer") {
        JApplication app;
        app.ProvideService<JTopologyBuilder>(topology); // Override the builtin one

        REQUIRE(sum_everything->sum == 0);

        app.Run(true);
        auto scheduler = app.GetService<JArrowProcessingController>()->get_scheduler();

        auto ts = scheduler->get_topology_state();
        REQUIRE(ts.current_topology_status == JScheduler::TopologyStatus::Finalized);
        REQUIRE(ts.arrow_states[0].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(sum_everything->sum == 20 * 13);

    }
}
