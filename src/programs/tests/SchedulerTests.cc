//
// Created by nbrei on 3/29/19.
//

#include "catch.hpp"

#include <greenfield/Scheduler.h>
#include <greenfield/ExampleComponents.h>
#include <greenfield/LinearTopologyBuilder.h>

namespace greenfield {
TEST_CASE("SchedulerTests") {

    SumSink<double> sink;
    Topology topology;
    LinearTopologyBuilder builder(topology);
    builder.addSource<RandIntSource>("emit_rand_ints");
    builder.addProcessor<MultByTwoProcessor>("multiply_by_two");
    builder.addProcessor<SubOneProcessor>("subtract_one");
    builder.addSink("sum_everything", sink);

    topology.activate("emit_rand_ints");

    Arrow* assignment;
    StreamStatus last_result;


    SECTION("When run sequentially, RRS returns nullptr => topology finished") {

        auto logger = Logger::nothing(); // everything();

        RoundRobinScheduler scheduler(topology);
        scheduler.logger = logger;

        last_result = StreamStatus::ComeBackLater;
        assignment = nullptr;
        do {
            assignment = scheduler.next_assignment(0, assignment, last_result);
            if (assignment != nullptr) {
                last_result = assignment->execute();
            }
        } while (assignment != nullptr);

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);
    }

    SECTION("When run sequentially, topology finished => RRS returns nullptr") {

        auto logger = Logger::nothing();
        RoundRobinScheduler scheduler(topology);
        scheduler.logger = logger;
        last_result = StreamStatus::ComeBackLater;
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
                last_result = assignment->execute();
            }
        }
        assignment = scheduler.next_assignment(0, assignment, last_result);
        REQUIRE(assignment == nullptr);
    }

    SECTION("When run sequentially, RRS returns nullptr => topology finished") {

        auto logger = Logger::nothing(); // everything();

        RoundRobinScheduler scheduler(topology);
        scheduler.logger = logger;

        last_result = StreamStatus::ComeBackLater;
        assignment = nullptr;
        do {
            assignment = scheduler.next_assignment(0, assignment, last_result);
            if (assignment != nullptr) {
                last_result = assignment->execute();
            }
        } while (assignment != nullptr);

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);
    }

    SECTION("When run sequentially, IndependentScheduler returns nullptr => topology finished") {

        auto logger = Logger::nothing(); // everything();

        IndependentScheduler scheduler(topology);
        scheduler.logger = logger;

        last_result = StreamStatus::ComeBackLater;
        assignment = nullptr;
        do {
            assignment = scheduler.next_assignment(0, assignment, last_result);
            if (assignment != nullptr) {
                last_result = assignment->execute();
            }
        } while (assignment != nullptr);

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);
    }
    SECTION("When run sequentially, topology finished => IndependentScheduler returns nullptr") {

        auto logger = Logger::nothing();
        IndependentScheduler scheduler(topology);
        scheduler.logger = logger;
        last_result = StreamStatus::ComeBackLater;
        assignment = nullptr;
        Arrow *assignment = nullptr;

        bool keep_going = true;
        while (keep_going) {

            keep_going = false;
            auto statuses = topology.get_arrow_status();
            for (auto status : statuses) {
                keep_going |= status.is_active;
            }

            if (keep_going) {
                assignment = scheduler.next_assignment(0, assignment, last_result);
                last_result = assignment->execute();
            }
        }
        assignment = scheduler.next_assignment(0, assignment, last_result);
        REQUIRE(assignment == nullptr);
    }

    SECTION("When run sequentially, FixedScheduler returns nullptr <=> arrow is finished") {

        Logger logger = Logger::nothing();

        std::map<std::string, int> assignments;
        assignments["emit_rand_ints"] = 1;
        assignments["multiply_by_two"] = 1;
        assignments["subtract_one"] = 1;
        assignments["sum_everything"] = 1;

        FixedScheduler scheduler(topology, assignments);
        scheduler.logger = logger;
        topology.logger = logger;

        Arrow* worker_assignments[4];
        StreamStatus worker_results[4];

        for (int worker=0; worker<4; worker++) {
            worker_assignments[worker] = nullptr;
            worker_results[worker] = StreamStatus::ComeBackLater;
        }

        for (int i=0; i<80; ++i) {
            REQUIRE(topology.is_active());

            topology.log_status();
            int worker = i % 4;
            Arrow *arr = scheduler.next_assignment(worker, worker_assignments[worker], worker_results[worker]);
            if (worker_assignments[worker] != nullptr) {
                REQUIRE(worker_assignments[worker]->is_active());
                REQUIRE(arr != nullptr);
            }

            if (arr != nullptr) {
                worker_assignments[worker] = arr;
                worker_results[worker] = arr->execute();
            }
        }

        for (int i=0; i<8; ++i) {
            REQUIRE(!topology.is_active());

            topology.log_status();
            int worker = i%4;
            Arrow *arr = scheduler.next_assignment(worker, worker_assignments[worker], worker_results[worker]);
            if (worker_assignments[worker] != nullptr) {
                REQUIRE(!worker_assignments[worker]->is_active());
                REQUIRE(arr == nullptr);
            }

            if (arr != nullptr) {
                worker_results[worker] = arr->execute();
                worker_assignments[worker] = arr;
            }
        }

    }
}
}