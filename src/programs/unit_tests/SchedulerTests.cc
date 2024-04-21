
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"

#include <JANA/Engine/JScheduler.h>
#include <TestTopologyComponents.h>
#include <JANA/Topology/JTopologyBuilder.h>

TEST_CASE("SchedulerTests") {

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
    
    scheduler.run_topology(1);

    JArrow* assignment;
    JArrowMetrics::Status last_result;


    SECTION("When run sequentially, RRS returns nullptr => topology finished") {

        last_result = JArrowMetrics::Status::ComeBackLater;
        assignment = nullptr;
        do {
            assignment = scheduler.next_assignment(0, assignment, last_result);
            if (assignment != nullptr) {
                JArrowMetrics metrics;
                assignment->execute(metrics, 0);
                last_result = metrics.get_last_status();
            }
        } while (assignment != nullptr);

        JScheduler::TopologyState state = scheduler.get_topology_state();
        REQUIRE(state.arrow_states[0].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(state.arrow_states[1].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(state.arrow_states[2].status == JScheduler::ArrowStatus::Finalized);
        REQUIRE(state.arrow_states[3].status == JScheduler::ArrowStatus::Finalized);
    }

    SECTION("When run sequentially, topology finished => RRS returns nullptr") {

        last_result = JArrowMetrics::Status::ComeBackLater;
        assignment = nullptr;

        for (int i=0; i<80; ++i) {
            // 20 events in source which need to pass through 4 arrows

            assignment = scheduler.next_assignment(0, assignment, last_result);
            REQUIRE(assignment != nullptr);
            JArrowMetrics metrics;
            assignment->execute(metrics, 0);
            last_result = metrics.get_last_status();
        }
        assignment = scheduler.next_assignment(0, assignment, last_result);
        REQUIRE(assignment == nullptr);
    }
}

TEST_CASE("SchedulerRoundRobinBehaviorTests") {

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
    scheduler.run_topology(1);

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
            std::cout << "Assignment is " << assignment->get_name() << std::endl;
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
        REQUIRE(sum_everything_arrow != nullptr);
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

}


