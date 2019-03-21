#pragma once

#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>


namespace greenfield {

    struct Topology {

        // Usage:

        // auto q0 = topology.addQueue(new Queue<int>);
        // auto q1 = topology.addQueue(new Queue<double>);

        // topology.addArrow("emit_rand_ints", new RandIntSourceArrow(q0));
        // topology.addArrow("multiply_by_two", new MultByTwoArrow(q0, q1));
        // topology.addArrow("sum_everything", new SumArrow<double>(q1));

        // ThreadManager.submit(topology);


        // TODO: Consider using shared_ptr instead of raw pointer
        // TODO: Figure out access control

        // #####################################################
        // These are meant to be used by ThreadManager
        std::map<std::string, Arrow*> arrows;
        std::vector<QueueBase*> queues;

        // #####################################################
        // These are meant to be used by the user
        QueueBase* addQueue(QueueBase* queue) {
            queues.push_back(queue);
            return queue;
        }

        void addArrow(std::string name, Arrow* arrow) {
            // Note that arrow name lives on the Topology, not on the Arrow
            // itself: different instances of the same Arrow can
            // be assigned different places in the same Topology, but
            // the users are inevitably going to make the Arrow
            // name be constant w.r.t to the Arrow class unless we force
            // them to do it correctly.

            arrows[name] = arrow;
            arrow->set_name(name);
        };

        /// The user may want to pause the topology and interact with it manually.
        /// This is particularly powerful when used from inside GDB.
        SchedulerHint step(const std::string& arrow_name) {
            Arrow* arrow = arrows[arrow_name];
            if (arrow == nullptr) {
                return SchedulerHint::Error;
            }
            return arrow->execute();
        }
    };
}



