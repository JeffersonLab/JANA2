
#include "catch.hpp"

#include <thread>
#include <random>
#include <greenfield/Topology.h>
#include "greenfield/TopologyTestFixtures.h"


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

        auto sumArrow = new SumArrow<double>(q2);
        topology.addArrow("sum_everything", sumArrow);

        auto logger = JLogger::everything();
        topology.logger = logger;


        SECTION("Before anything runs...") {

            // All queues are empty, none are finished
            for (QueueBase* queue : topology.queues) {
                REQUIRE (queue->get_item_count() == 0);
                REQUIRE (! queue->is_finished());
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
                REQUIRE (! queue->is_finished());
            }
        }

        SECTION("After emitting") {
            //LOG_INFO(logger) << "After emitting; should be something in q0" << LOG_END;

            //topology.log_queue_status();
            topology.arrows["emit_rand_ints"]->execute();
            //topology.log_queue_status();

            REQUIRE(topology.queues[0]->get_item_count() == 1);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 0);

            topology.step("emit_rand_ints");
            topology.step("emit_rand_ints");
            topology.step("emit_rand_ints");
            topology.step("emit_rand_ints");
            //topology.log_queue_status();

            REQUIRE(topology.queues[0]->get_item_count() == 5);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 0);
        }

        SECTION("Running each stage sequentially yields the correct results") {

            //LOG_INFO(logger) << "Running each stage sequentially yields the correct results" << LOG_END;

            for (int i=0; i<20; ++i) {
                topology.arrows["emit_rand_ints"]->execute();
                REQUIRE(topology.queues[0]->get_item_count() == 1);
                REQUIRE(topology.queues[1]->get_item_count() == 0);
                REQUIRE(topology.queues[2]->get_item_count() == 0);

                topology.arrows["multiply_by_two"]->execute();
                REQUIRE(topology.queues[0]->get_item_count() == 0);
                REQUIRE(topology.queues[1]->get_item_count() == 1);
                REQUIRE(topology.queues[2]->get_item_count() == 0);

                topology.arrows["subtract_one"]->execute();
                REQUIRE(topology.queues[0]->get_item_count() == 0);
                REQUIRE(topology.queues[1]->get_item_count() == 0);
                REQUIRE(topology.queues[2]->get_item_count() == 1);

                topology.arrows["sum_everything"]->execute();
                REQUIRE(topology.queues[0]->get_item_count() == 0);
                REQUIRE(topology.queues[1]->get_item_count() == 0);
                REQUIRE(topology.queues[2]->get_item_count() == 0);
            }

            REQUIRE(sumArrow->sum == (7 * 2.0 - 1) * 20);
        }

        SECTION("Running each stage in random order (sequentially) yields the correct results") {
            //LOG_INFO(logger) << "Running each stage in arbitrary order yields the correct results" << LOG_END;

            Arrow* arrows[] = {topology.arrows["emit_rand_ints"],
                               topology.arrows["multiply_by_two"],
                               topology.arrows["subtract_one"],
                               topology.arrows["sum_everything"]};

            std::map<std::string, SchedulerHint> results;
            results["emit_rand_ints"] = SchedulerHint::KeepGoing;
            results["multiply_by_two"] = SchedulerHint::KeepGoing;
            results["subtract_one"] = SchedulerHint::KeepGoing;
            results["sum_everything"] = SchedulerHint::KeepGoing;

            // Put something in the queue to get started
            topology.arrows["emit_rand_ints"]->execute();

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
                    if (pair.second == SchedulerHint::KeepGoing) { work_left = true; }
                }
            }

            //topology.log_queue_status();
            REQUIRE(sumArrow->sum == (7 * 2.0 - 1) * 20);
        }
    }
}
