
#include "catch.hpp"

#include <greenfield/Scheduler.h>
#include <greenfield/TopologyTestFixtures.h>

namespace greenfield {

    TEST_CASE("greenfield::RoundRobinScheduler") {

        auto logger = JLogger::nothing(); // everything();

        Topology topology;

        auto q0 = new Queue<int>;
        auto q1 = new Queue<double>;
        auto q2 = new Queue<double>;

        topology.addQueue(q0);
        topology.addQueue(q1);
        topology.addQueue(q2);

        topology.addArrow("emit_rand_ints", new RandIntSourceArrow(q0));
        topology.addArrow("multiply_by_two", new MultByTwoArrow(q0, q1));
        topology.addArrow("subtract_one", new SubOneArrow(q1, q2));
        topology.addArrow("sum_everything", new SumArrow<double>(q2));

        RoundRobinScheduler scheduler(topology);
        scheduler.logger = JLogger::nothing(); // everything();
        Scheduler::Report report;

        SECTION("When there is only one worker, who always encounters ComeBackLater...") {

            LOG_INFO(logger) << "---------------------------------------" << LOG_END;
            std::string ordering[] = {"emit_rand_ints",
                                      "multiply_by_two",
                                      "subtract_one",
                                      "sum_everything"};

            report.last_result = SchedulerHint::ComeBackLater;
            for (int i = 0; i < 10; ++i) {
                Arrow *assignment = scheduler.next_assignment(report);
                report.assignment = assignment;

                // The assignments go round-robin
                REQUIRE(assignment->get_name() == ordering[i % 4]);
            }
        }


        SECTION("When a team of workers start off with (nullptr, ComeBackLater)...") {

            LOG_INFO(logger) << "---------------------------------------" << LOG_END;
            std::map<std::string, int> assignment_counts;

            for (int i = 0; i < 10; ++i) {
                report.assignment = nullptr;
                report.last_result = SchedulerHint::ComeBackLater;
                Arrow *assignment = scheduler.next_assignment(report);
                assignment_counts[assignment->get_name()]++;

                // They all receive a nonnull assignment
                REQUIRE (assignment != nullptr);
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
            report.last_result = SchedulerHint::ComeBackLater;

            scheduler.next_assignment(report);
            scheduler.next_assignment(report);
            scheduler.next_assignment(report);
            auto sum_everything_arrow = scheduler.next_assignment(report);

            // Last assignment returned sequential arrow "sum_everything"
            REQUIRE(sum_everything_arrow == topology.arrows["sum_everything"]);

            std::map<std::string, int> assignment_counts;

            // We return the sequential arrow to the scheduler
            report.assignment = sum_everything_arrow;
            report.last_result = SchedulerHint::ComeBackLater;
            auto arrow = scheduler.next_assignment(report);

            assignment_counts[arrow->get_name()]++;

            for (int i = 0; i < 8; ++i) {
                report.assignment = nullptr;
                report.last_result = SchedulerHint::ComeBackLater;
                arrow = scheduler.next_assignment(report);
                assignment_counts[arrow->get_name()]++;
            }

            // Once scheduler receives a sequential arrow back, it will offer it back out
            // but only once, if we don't return it

            REQUIRE(assignment_counts["sum_everything"] == 1);
        }

        SECTION("When an arrow encountered KeepGoing...") {

            LOG_INFO(logger) << "---------------------------------------" << LOG_END;
            report.last_result = SchedulerHint::KeepGoing;
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


