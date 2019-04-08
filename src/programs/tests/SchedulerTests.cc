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
    Arrow::Status last_result;


    SECTION("When run sequentially, RRS returns nullptr => topology finished") {

        auto logger = Logger::nothing(); // everything();

        Scheduler scheduler(topology.arrows, 1);

        last_result = Arrow::Status::ComeBackLater;
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
        Scheduler scheduler(topology.arrows, 1);
        last_result = Arrow::Status::ComeBackLater;
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
}
}