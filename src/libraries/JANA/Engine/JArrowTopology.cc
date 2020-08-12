
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JArrowTopology.h"
#include "JEventProcessorArrow.h"
#include "JEventSourceArrow.h"

bool JArrowTopology::is_active() {
    for (auto arrow : arrows) {
        if (arrow->is_active()) {
            return true;
        }
    }
    return false;
}

void JArrowTopology::set_active(bool active) {
    if (active) {
        if (status == Status::Inactive) {
            status = Status::Running;
            for (auto arrow : sources) {
                // We activate our eventsources, which activate components downstream.
                arrow->initialize();
                arrow->set_active(true);
                arrow->notify_downstream(true);
            }
        }
    }
    else {
        // Someone has told us to deactivate. There are two ways to get here:
        //   * The last JEventProcessorArrow notifies us that he is deactivating because the topology is finished
        //   * The JArrowController is requesting us to stop regardless of whether the topology finished or not

        if (is_active()) {
            // Premature exit: Shut down any arrows which are still running
            status = Status::Draining;
            for (auto arrow : arrows) {
                arrow->set_active(false);
                arrow->finalize();
            }
        }
        else {
            // Arrows have all deactivated: Stop timer
            metrics.stop();
            status = Status::Inactive;
            for (auto arrow : arrows) {
                arrow->finalize();
            }
        }
    }
}

JArrowTopology::JArrowTopology() = default;

JArrowTopology::~JArrowTopology() {
    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto queue : queues) {
        delete queue;
    }
}
