//
// Created by nbrei on 3/29/19.
//

#include "catch.hpp"

#include <JANA/JScheduler.h>
#include <TestTopologyComponents.h>
#include <TestTopologyBuilder.h>

TEST_CASE("SchedulerTests") {

    SumSink<double> sink;
    TestTopology topology;
    TestTopologyBuilder builder(topology);
    builder.addSource<RandIntSource>("emit_rand_ints");
    builder.addProcessor<MultByTwoProcessor>("multiply_by_two");
    builder.addProcessor<SubOneProcessor>("subtract_one");
    builder.addSink("sum_everything", sink);

    topology.activate("emit_rand_ints");

    JArrow* assignment;
    JArrowMetrics::Status last_result;


    SECTION("When run sequentially, RRS returns nullptr => topology finished") {

        auto logger = JLogger::nothing(); // everything();

        JScheduler scheduler(topology.arrows);

        last_result = JArrowMetrics::Status::ComeBackLater;
        assignment = nullptr;
        do {
            assignment = scheduler.next_assignment(0, assignment, last_result);
            if (assignment != nullptr) {
                JArrowMetrics metrics;
                assignment->execute(metrics);
                last_result = metrics.get_last_status();
            }
        } while (assignment != nullptr);

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);
    }

    SECTION("When run sequentially, topology finished => RRS returns nullptr") {

        auto logger = JLogger::nothing();
        JScheduler scheduler(topology.arrows);
        last_result = JArrowMetrics::Status::ComeBackLater;
        assignment = nullptr;

        bool keep_going = true;
        while (keep_going) {

            keep_going = false;
            auto statuses = topology.get_arrow_status();
            for (auto status : statuses) {
                keep_going |= status.is_active;
            }

            if (keep_going) {
                assignment = scheduler.next_assignment(0, assignment, last_result);
                JArrowMetrics metrics;
                assignment->execute(metrics);
                last_result = metrics.get_last_status();
            }
        }
        assignment = scheduler.next_assignment(0, assignment, last_result);
        REQUIRE(assignment == nullptr);
    }
}

TEST_CASE("SchedulerRoundRobinBehaviorTests") {

    SumSink<double> sink;

    TestTopology topology;
    TestTopologyBuilder builder(topology);

    builder.addSource<RandIntSource>("emit_rand_ints");
    builder.addProcessor<MultByTwoProcessor>("multiply_by_two");
    builder.addProcessor<SubOneProcessor>("subtract_one");
    builder.addSink("sum_everything", sink);

    JScheduler scheduler(topology.arrows);

    auto logger = JLogger::nothing(); // everything();

    // Assume everything is active for now
    topology.activate("emit_rand_ints");

    auto last_result = JArrowMetrics::Status::ComeBackLater;
    JArrow* assignment = nullptr;

    SECTION("When there is only one worker, who always encounters ComeBackLater...") {

        LOG_INFO(logger) << "---------------------------------------" << LOG_END;
        std::string ordering[] = {"emit_rand_ints",
                                  "multiply_by_two",
                                  "subtract_one",
                                  "sum_everything"};

        for (int i = 0; i < 10; ++i) {
            assignment = scheduler.next_assignment(0, assignment, last_result);

            // The assignments go round-robin
            REQUIRE(assignment != nullptr);
            REQUIRE(assignment->get_name() == ordering[i % 4]);
        }
    }


    SECTION("When a team of workers start off with (nullptr, ComeBackLater)...") {

        LOG_INFO(logger) << "---------------------------------------" << LOG_END;
        std::map<std::string, int> assignment_counts;


        for (int i = 0; i < 10; ++i) {
            assignment = nullptr;
            last_result = JArrowMetrics::Status::ComeBackLater;
            assignment = scheduler.next_assignment(i, assignment, last_result);

            // They all receive a nonnull assignment
            REQUIRE (assignment != nullptr);

            assignment_counts[assignment->get_name()]++;
        }

        // The sequential arrows only get assigned once
        REQUIRE(assignment_counts["emit_rand_ints"] == 1);
        REQUIRE(assignment_counts["sum_everything"] == 1);

        // The parallel arrows get assigned many times, evenly
        REQUIRE(assignment_counts["subtract_one"] == 4);
        REQUIRE(assignment_counts["multiply_by_two"] == 4);

    }


    SECTION("When all arrows are assigned, and a sequential arrow comes back...") {


        LOG_INFO(logger) << "--------------------------------------" << LOG_END;

        scheduler.next_assignment(0, nullptr, JArrowMetrics::Status::ComeBackLater);
        scheduler.next_assignment(1, nullptr, JArrowMetrics::Status::ComeBackLater);
        scheduler.next_assignment(2, nullptr, JArrowMetrics::Status::ComeBackLater);
        auto sum_everything_arrow = scheduler.next_assignment(3, nullptr, JArrowMetrics::Status::ComeBackLater);

        // Last assignment returned sequential arrow "sum_everything"
        REQUIRE(sum_everything_arrow->get_name() == "sum_everything");

        std::map<std::string, int> assignment_counts;

        // We return the sequential arrow to the scheduler
        assignment = sum_everything_arrow;
        last_result = JArrowMetrics::Status::ComeBackLater;
        auto arrow = scheduler.next_assignment(3, assignment, last_result);

        assignment_counts[arrow->get_name()]++;

        for (int i = 0; i < 8; ++i) {
            assignment = nullptr;
            last_result = JArrowMetrics::Status::ComeBackLater;
            arrow = scheduler.next_assignment(i, assignment, last_result);
            assignment_counts[arrow->get_name()]++;
        }

        // Once scheduler receives a sequential arrow back, it will offer it back out
        // but only once, if we don't return it

        REQUIRE(assignment_counts["sum_everything"] == 1);
    }

    SECTION("When an arrow encountered KeepGoing...") {

        LOG_INFO(logger) << "--------------------------------------" << LOG_END;
        last_result = JArrowMetrics::Status::KeepGoing;
        assignment = topology.get_arrow("subtract_one");

        for (int i = 0; i < 4; ++i) {
            assignment = scheduler.next_assignment(0, assignment, last_result);

            REQUIRE(assignment->get_name() == "subtract_one");
            // That scheduler lets the worker continue with this assignment
        }

    }
}

