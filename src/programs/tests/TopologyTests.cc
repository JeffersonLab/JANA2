
#include "catch.hpp"
#include <greenfield/Topology.h>
#include "greenfield/TopologyTestFixtures.h"


namespace greenfield {

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
        topology.addArrow("sum_everything", new SumArrow<double>(q2));

        auto logger = JLogger::everything();
        topology.logger = logger;


        SECTION("Before anything runs...") {

            // All queues are empty, none are finished
            for (QueueBase* queue : topology.queues) {
                REQUIRE (queue->get_item_count() == 0);
                REQUIRE (! queue->is_finished());
            }
            LOG_INFO(logger) << "Nothing has run at all; should be empty" << LOG_END;
            topology.log_queue_status();
        }

        SECTION("When nothing is in the input queue...") {

            LOG_INFO(logger) << "Nothing has run yet; should be empty" << LOG_END;
            topology.log_queue_status();

            topology.arrows["multiply_by_two"]->execute();
            topology.log_queue_status();
            topology.arrows["subtract_one"]->execute();
            topology.log_queue_status();
            topology.arrows["sum_everything"]->execute();
            topology.log_queue_status();
            topology.arrows["multiply_by_two"]->execute();
            topology.log_queue_status();
            topology.arrows["subtract_one"]->execute();
            topology.log_queue_status();
            topology.arrows["sum_everything"]->execute();
            topology.log_queue_status();

            LOG_INFO(logger) << "Nothing in input queue; should be empty" << LOG_END;
            topology.log_queue_status();
            // All `execute` operations are no-ops
            for (QueueBase* queue : topology.queues) {
                //REQUIRE (queue->get_item_count() == 0);
                REQUIRE (! queue->is_finished());
            }
            std::vector<double> items;
            q2->pop(items, 100);
            for (double item: items) {
                LOG_ERROR(logger) << "This shouldn't be here: " << item << LOG_END;
            }
        }

        SECTION("After emitting") {
            topology.arrows["emit_rand_ints"]->execute();

            LOG_INFO(logger) << "After emitting; should be something in q0" << LOG_END;
            topology.log_queue_status();

            REQUIRE(topology.queues[0]->get_item_count() != 0);
            REQUIRE(topology.queues[1]->get_item_count() == 0);
            REQUIRE(topology.queues[2]->get_item_count() == 0);

        }

    }
}
