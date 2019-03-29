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
    LinearTopologyBuilder builder;
    builder.addSource<RandIntSource>("emit_rand_ints");
    builder.addProcessor<MultByTwoProcessor>("multiply_by_two");
    builder.addProcessor<SubOneProcessor>("subtract_one");
    builder.addSink("sum_everything", sink);
    auto topology = builder.get();
    topology.activate("emit_rand_ints");
    Scheduler::Report report;


    SECTION("When run sequentially, RRS returns nullptr => topology finished") {

        auto logger = Logger::nothing(); // everything();

        RoundRobinScheduler scheduler(topology);
        scheduler.logger = logger;

        report.last_result = StreamStatus::ComeBackLater;
        report.assignment = nullptr;
        do {
            report.assignment = scheduler.next_assignment(report);
            if (report.assignment != nullptr) {
                report.last_result = report.assignment->execute();
            }
        } while (report.assignment != nullptr);

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);
    }

    SECTION("When run sequentially, topology finished => RRS returns nullptr") {

        auto logger = Logger::nothing();
        RoundRobinScheduler scheduler(topology);
        scheduler.logger = logger;
        report.last_result = StreamStatus::ComeBackLater;
        report.assignment = nullptr;
        Arrow *assignment = nullptr;

        bool keep_going = true;
        while (keep_going) {

            keep_going = false;
            auto statuses = topology.get_arrow_status();
            for (auto status : statuses) {
                keep_going |= status.is_active;
            }

            if (keep_going) {
                assignment = scheduler.next_assignment(report);
                report.assignment = assignment;
                report.last_result = assignment->execute();
            }
        }
        assignment = scheduler.next_assignment(report);
        REQUIRE(assignment == nullptr);
    }

    SECTION("When run sequentially, RRS returns nullptr => topology finished") {

        auto logger = Logger::nothing(); // everything();

        RoundRobinScheduler scheduler(topology);
        scheduler.logger = logger;

        report.last_result = StreamStatus::ComeBackLater;
        report.assignment = nullptr;
        do {
            report.assignment = scheduler.next_assignment(report);
            if (report.assignment != nullptr) {
                report.last_result = report.assignment->execute();
            }
        } while (report.assignment != nullptr);

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);
    }

    SECTION("When run sequentially, IndependentScheduler returns nullptr => topology finished") {

        auto logger = Logger::nothing(); // everything();

        IndependentScheduler scheduler(topology);
        scheduler.logger = logger;

        report.last_result = StreamStatus::ComeBackLater;
        report.assignment = nullptr;
        do {
            report.assignment = scheduler.next_assignment(report);
            if (report.assignment != nullptr) {
                report.last_result = report.assignment->execute();
            }
        } while (report.assignment != nullptr);

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);
    }
    SECTION("When run sequentially, topology finished => IndependentScheduler returns nullptr") {

        auto logger = Logger::nothing();
        IndependentScheduler scheduler(topology);
        scheduler.logger = logger;
        report.last_result = StreamStatus::ComeBackLater;
        report.assignment = nullptr;
        Arrow *assignment = nullptr;

        bool keep_going = true;
        while (keep_going) {

            keep_going = false;
            auto statuses = topology.get_arrow_status();
            for (auto status : statuses) {
                keep_going |= status.is_active;
            }

            if (keep_going) {
                assignment = scheduler.next_assignment(report);
                report.assignment = assignment;
                report.last_result = assignment->execute();
            }
        }
        assignment = scheduler.next_assignment(report);
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

        Scheduler::Report reports[4];
        for (int worker=0; worker<4; worker++) {
            reports[worker].worker_id = worker;
            reports[worker].assignment = nullptr;
            reports[worker].last_result = StreamStatus::ComeBackLater;
        }

        for (int i=0; i<80; ++i) {
            REQUIRE(topology.is_active());

            topology.log_status();
            Arrow *assignment = scheduler.next_assignment(reports[i%4]);
            if (reports[i%4].assignment != nullptr) {
                REQUIRE(reports[i%4].assignment->is_active());
                REQUIRE(assignment != nullptr);
            }

            if (assignment != nullptr) {
                reports[i%4].last_result = assignment->execute();
                reports[i%4].assignment = assignment;
            }
        }

        for (int i=0; i<8; ++i) {
            REQUIRE(!topology.is_active());

            topology.log_status();
            Arrow *assignment = scheduler.next_assignment(reports[i%4]);
            if (reports[i%4].assignment != nullptr) {
                REQUIRE(!reports[i%4].assignment->is_active());
                REQUIRE(assignment == nullptr);
            }

            if (assignment != nullptr) {
                reports[i%4].last_result = assignment->execute();
                reports[i%4].assignment = assignment;
            }
        }

    }
}
}