
#include "catch.hpp"

#include <thread>
#include <random>
#include <greenfield/Topology.h>
#include <greenfield/LinearTopologyBuilder.h>
#include "greenfield/ExampleComponents.h"


namespace greenfield {


    static thread_local std::mt19937* generator = nullptr;

    int randint(int min, int max) {

        std::hash<std::thread::id> hasher;
        long seed = clock() + hasher(std::this_thread::get_id());
        if (!generator) generator = new std::mt19937(seed);
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(*generator);
    }

    TEST_CASE("greenfield::Topology: Basic functionality") {

        LinearTopologyBuilder b;
        RandIntSource source;
        MultByTwoProcessor p1;
        SubOneProcessor p2;
        SumSink<double> sink;

        b.addSource("emit_rand_ints", source);
        b.addProcessor("multiply_by_two", p1);
        b.addProcessor("subtract_one", p2);
        b.addSink("sum_everything", sink);

        auto topology = b.get();

        auto logger = JLogger::nothing();
        //topology.logger = JLogger::everything();
        //source.logger = JLogger::everything();


        SECTION("Before anything runs...") {

            // All queues are empty, none are finished
            for (QueueBase* queue : topology.queues) {
                REQUIRE (queue->get_item_count() == 0);
            }
        }

        SECTION("When nothing is in the input queue...") {

            //LOG_INFO(logger) << "Nothing has run yet; should be empty" << LOG_END;

            //topology.log_queue_status();
            topology.arrows["multiply_by_two"]->execute();
            topology.arrows["subtract_one"]->execute();
            topology.arrows["sum_everything"]->execute();
            topology.arrows["multiply_by_two"]->execute();
            topology.arrows["subtract_one"]->execute();
            topology.arrows["sum_everything"]->execute();
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

            topology.log_queue_status();
            topology.activate("emit_rand_ints");
            topology.step("emit_rand_ints");
            topology.log_queue_status();

            REQUIRE(topology.queues[0]->get_item_count() == 1);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 0);

            topology.step("emit_rand_ints");
            topology.step("emit_rand_ints");
            topology.step("emit_rand_ints");
            topology.step("emit_rand_ints");
            topology.log_queue_status();

            REQUIRE(topology.queues[0]->get_item_count() == 5);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 0);
        }

        SECTION("Running each stage sequentially yields the correct results") {

            //LOG_INFO(logger) << "Running each stage sequentially yields the correct results" << LOG_END;
            topology.activate("emit_rand_ints");

            for (int i=0; i<20; ++i) {
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

            Arrow* arrows[] = {topology.arrows["emit_rand_ints"],
                               topology.arrows["multiply_by_two"],
                               topology.arrows["subtract_one"],
                               topology.arrows["sum_everything"]};

            std::map<std::string, StreamStatus> results;
            results["emit_rand_ints"] = StreamStatus::KeepGoing;
            results["multiply_by_two"] = StreamStatus::KeepGoing;
            results["subtract_one"] = StreamStatus::KeepGoing;
            results["sum_everything"] = StreamStatus::KeepGoing;

            // Put something in the queue to get started
            topology.activate("emit_rand_ints");
            topology.step("emit_rand_ints");

            bool work_left = true;
            while (work_left) {
                // Pick a random arrow
                Arrow* arrow = arrows[randint(0,3)];

                auto name = arrow->get_name();
                auto res = arrow->execute();
                results[name] = res;
                //LOG_TRACE(logger) << name << " => "
                //                  << to_string(res) << LOG_END;

                work_left = false;
                for (QueueBase * queue : topology.queues) {
                    if (queue->get_item_count() > 0) {
                        work_left = true;
                    }
                }
                for (auto pair : results) {
                    if (pair.second == StreamStatus::KeepGoing) { work_left = true; }
                }
            }

            //topology.log_queue_status();
            REQUIRE(sink.sum == (7 * 2.0 - 1) * 20);
        }
        SECTION("Finished flag propagates") {

            logger = JLogger::everything();
            topology.logger = JLogger::everything();
            source.logger = JLogger::everything();

            topology.finalize();
            topology.activate("emit_rand_ints");
            topology.activate("multiply_by_two");
            topology.activate("subtract_one");
            topology.activate("sum_everything");

            REQUIRE(topology.arrows["emit_rand_ints"]->is_active() == true);
            REQUIRE(topology.arrows["multiply_by_two"]->is_active() == true);
            REQUIRE(topology.arrows["subtract_one"]->is_active() == true);
            REQUIRE(topology.arrows["sum_everything"]->is_active() == true);

            for (int i=0; i<20; ++i) {
                topology.step("emit_rand_ints");
            }

            REQUIRE(topology.arrows["emit_rand_ints"]->is_active() == false);
            REQUIRE(topology.arrows["multiply_by_two"]->is_active() == true);
            REQUIRE(topology.arrows["subtract_one"]->is_active() == true);
            REQUIRE(topology.arrows["sum_everything"]->is_active() == true);


            for (int i=0; i<20; ++i) {
                topology.step("multiply_by_two");
            }

            topology.log_arrow_status();
            topology.log_queue_status();
            // TODO: Finished flag doesn't propagate correctly past this point yet
//
//            auto arrow_statuses = topology.get_arrow_status();
//            REQUIRE(arrow_statuses[0].is_finished == true);
//            REQUIRE(arrow_statuses[1].is_finished == true);
//            REQUIRE(arrow_statuses[2].is_finished == false);
//            REQUIRE(arrow_statuses[3].is_finished == false);
//
//            for (int i=0; i<20; ++i) {
//                topology.step("subtract_one");
//            }
//
//            topology.log_arrow_status();
//            topology.log_queue_status();
//
//
//            arrow_statuses = topology.get_arrow_status();
//            REQUIRE(arrow_statuses[0].is_finished == true);
//            REQUIRE(arrow_statuses[1].is_finished == true);
//            REQUIRE(arrow_statuses[2].is_finished == true);
//            REQUIRE(arrow_statuses[3].is_finished == false);
        }
    }
}
