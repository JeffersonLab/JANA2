
#include "catch.hpp"

#include <greenfield/Scheduler.h>
#include <greenfield/ExampleComponents.h>
#include <greenfield/LinearTopologyBuilder.h>

namespace greenfield {

TEST_CASE("greenfield::RoundRobinScheduler") {

    SumSink<double> sink;

    LinearTopologyBuilder builder;

    builder.addSource<RandIntSource>("emit_rand_ints");
    builder.addProcessor<MultByTwoProcessor>("multiply_by_two");
    builder.addProcessor<SubOneProcessor>("subtract_one");
    builder.addSink("sum_everything", sink);

    auto topology = builder.get();

    RoundRobinScheduler scheduler(topology);

    auto logger = Logger::nothing(); // everything();
    scheduler.logger = Logger::nothing(); // everything();
    Scheduler::Report report;

    // Assume everything is active for now
    topology.activate("emit_rand_ints");

    SECTION("When there is only one worker, who always encounters ComeBackLater...") {

        LOG_INFO(logger) << "---------------------------------------" << LOG_END;
        std::string ordering[] = {"emit_rand_ints",
                                  "multiply_by_two",
                                  "subtract_one",
                                  "sum_everything"};

        report.last_result = StreamStatus::ComeBackLater;
        for (int i = 0; i < 10; ++i) {
            Arrow *assignment = scheduler.next_assignment(report);
            report.assignment = assignment;

            // The assignments go round-robin
            REQUIRE(assignment != nullptr);
            REQUIRE(assignment->get_name() == ordering[i % 4]);
        }
    }


    SECTION("When a team of workers start off with (nullptr, ComeBackLater)...") {

        LOG_INFO(logger) << "---------------------------------------" << LOG_END;
        std::map<std::string, int> assignment_counts;


        for (int i = 0; i < 10; ++i) {
            report.assignment = nullptr;
            report.last_result = StreamStatus::ComeBackLater;
            Arrow *assignment = scheduler.next_assignment(report);

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


        LOG_INFO(logger) << "---------------------------------------" << LOG_END;

        report.assignment = nullptr;
        report.last_result = StreamStatus::ComeBackLater;

        scheduler.next_assignment(report);
        scheduler.next_assignment(report);
        scheduler.next_assignment(report);
        auto sum_everything_arrow = scheduler.next_assignment(report);

        // Last assignment returned sequential arrow "sum_everything"
        REQUIRE(sum_everything_arrow == topology.arrows["sum_everything"]);

        std::map<std::string, int> assignment_counts;

        // We return the sequential arrow to the scheduler
        report.assignment = sum_everything_arrow;
        report.last_result = StreamStatus::ComeBackLater;
        auto arrow = scheduler.next_assignment(report);

        assignment_counts[arrow->get_name()]++;

        for (int i = 0; i < 8; ++i) {
            report.assignment = nullptr;
            report.last_result = StreamStatus::ComeBackLater;
            arrow = scheduler.next_assignment(report);
            assignment_counts[arrow->get_name()]++;
        }

        // Once scheduler receives a sequential arrow back, it will offer it back out
        // but only once, if we don't return it

        REQUIRE(assignment_counts["sum_everything"] == 1);
    }

    SECTION("When an arrow encountered KeepGoing...") {

        LOG_INFO(logger) << "---------------------------------------" << LOG_END;
        report.last_result = StreamStatus::KeepGoing;
        report.assignment = topology.arrows["subtract_one"];

        for (int i = 0; i < 4; ++i) {
            Arrow *assignment = scheduler.next_assignment(report);
            report.assignment = assignment;

            REQUIRE(assignment->get_name() == "subtract_one");
            // That scheduler lets the worker continue with this assignment
        }

    }
}
}


