
#include "catch.hpp"

#include <thread>
#include <random>
#include <TestTopology.h>
#include <TestTopologyBuilder.h>
#include <TestTopologyComponents.h>
#include <JANA/JPerfUtils.h>


TEST_CASE("JTopology: Basic functionality") {

    TestTopology topology;
    TestTopologyBuilder b(topology);
    RandIntSource source;
    MultByTwoProcessor p1;
    SubOneProcessor p2;
    SumSink<double> sink;

    b.addSource("emit_rand_ints", source);
    b.addProcessor("multiply_by_two", p1);
    b.addProcessor("subtract_one", p2);
    b.addSink("sum_everything", sink);

    auto logger = JLogger::nothing();
    //topology.logger = Logger::everything();
    //source.logger = Logger::everything();


    SECTION("Before anything runs...") {

        // All queues are empty, none are finished
        for (QueueBase* queue : topology.queues) {
            REQUIRE (queue->get_item_count() == 0);
        }
    }

    SECTION("When nothing is in the input queue...") {

        //LOG_INFO(logger) << "Nothing has run yet; should be empty" << LOG_END;

        //topology.log_queue_status();
        topology.step("multiply_by_two");
        topology.step("subtract_one");
        topology.step("sum_everything");
        topology.step("multiply_by_two");
        topology.step("subtract_one");
        topology.step("sum_everything");
        //topology.log_queue_status();

        // All `execute` operations are no-ops
        for (QueueBase* queue : topology.queues) {
            REQUIRE (queue->get_item_count() == 0);
        }
    }

    SECTION("After emitting") {
        logger = JLogger::nothing();
        topology.logger = logger;
        source.logger = logger;

        //LOG_INFO(logger) << "After emitting; should be something in q0" << LOG_END;

        topology.log_status();
        topology.activate("emit_rand_ints");
        topology.step("emit_rand_ints");
        topology.log_status();

        REQUIRE(topology.queues[0]->get_item_count() == 1);
        REQUIRE(topology.queues[1]->get_item_count() == 0);
        REQUIRE(topology.queues[2]->get_item_count() == 0);

        topology.step("emit_rand_ints");
        topology.step("emit_rand_ints");
        topology.step("emit_rand_ints");
        topology.step("emit_rand_ints");
        topology.log_status();

        REQUIRE(topology.queues[0]->get_item_count() == 5);
        REQUIRE(topology.queues[1]->get_item_count() == 0);
        REQUIRE(topology.queues[2]->get_item_count() == 0);
    }

    SECTION("Running each stage sequentially yields the correct results") {

        //LOG_INFO(logger) << "Running each stage sequentially yields the correct results" << LOG_END;
        topology.activate("emit_rand_ints");

        for (int i = 0; i < 20; ++i) {
            topology.step("emit_rand_ints");
            REQUIRE(topology.queues[0]->get_item_count() == 1);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 0);

            topology.step("multiply_by_two");
            REQUIRE(topology.queues[0]->get_item_count() == 0);
            REQUIRE(topology.queues[1]->get_item_count() == 1);
            REQUIRE(topology.queues[2]->get_item_count() == 0);

            topology.step("subtract_one");
            REQUIRE(topology.queues[0]->get_item_count() == 0);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 1);

            topology.step("sum_everything");
            REQUIRE(topology.queues[0]->get_item_count() == 0);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 0);
        }

        REQUIRE(sink.sum == (7 * 2.0 - 1) * 20);
    }

    SECTION("Running each stage in random order (sequentially) yields the correct results") {
        //LOG_INFO(logger) << "Running each stage in arbitrary order yields the correct results" << LOG_END;

        JArrow* arrows[] = {topology.get_arrow("emit_rand_ints"),
                            topology.get_arrow("multiply_by_two"),
                            topology.get_arrow("subtract_one"),
                            topology.get_arrow("sum_everything")};

        std::map<std::string, JArrowMetrics::Status> results;
        results["emit_rand_ints"] = JArrowMetrics::Status::KeepGoing;
        results["multiply_by_two"] = JArrowMetrics::Status::KeepGoing;
        results["subtract_one"] = JArrowMetrics::Status::KeepGoing;
        results["sum_everything"] = JArrowMetrics::Status::KeepGoing;

        // Put something in the queue to get started
        topology.activate("emit_rand_ints");
        topology.step("emit_rand_ints");

        bool work_left = true;
        while (work_left) {
            // Pick a random arrow
            JArrow* arrow = arrows[randint(0, 3)];

            auto name = arrow->get_name();
            JArrowMetrics metrics;
            arrow->execute(metrics);
            auto res = metrics.get_last_status();
            results[name] = res;
            //LOG_TRACE(logger) << name << " => "
            //                  << to_string(res) << LOG_END;

            work_left = false;
            for (QueueBase* queue : topology.queues) {
                if (queue->get_item_count() > 0) {
                    work_left = true;
                }
            }
            for (auto pair : results) {
                if (pair.second == JArrowMetrics::Status::KeepGoing) { work_left = true; }
            }
        }

        //topology.log_queue_status();
        REQUIRE(sink.sum == (7 * 2.0 - 1) * 20);
    }
    SECTION("Finished flag propagates") {

        logger = JLogger::nothing();
        topology.logger = logger;
        source.logger = logger;

        topology.activate("emit_rand_ints");

        REQUIRE(topology.get_status("emit_rand_ints").is_active == true);
        REQUIRE(topology.get_status("multiply_by_two").is_active == true);
        REQUIRE(topology.get_status("subtract_one").is_active == true);
        REQUIRE(topology.get_status("sum_everything").is_active == true);

        for (int i = 0; i < 20; ++i) {
            topology.step("emit_rand_ints");
        }

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == true);
        REQUIRE(topology.get_status("subtract_one").is_active == true);
        REQUIRE(topology.get_status("sum_everything").is_active == true);

        for (int i = 0; i < 20; ++i) {
            topology.step("multiply_by_two");
        }

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == true);
        REQUIRE(topology.get_status("sum_everything").is_active == true);

        for (int i = 0; i < 20; ++i) {
            topology.step("subtract_one");
        }

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == true);

        for (int i = 0; i < 20; ++i) {
            topology.step("sum_everything");
        }

        REQUIRE(topology.get_status("emit_rand_ints").is_active == false);
        REQUIRE(topology.get_status("multiply_by_two").is_active == false);
        REQUIRE(topology.get_status("subtract_one").is_active == false);
        REQUIRE(topology.get_status("sum_everything").is_active == false);

        topology.log_status();

    }
}
