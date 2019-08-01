//
// Created by Nathan W Brei on 2019-04-24.
//

#include "JProcessingTopology.h"

bool JProcessingTopology::is_active() {
    for (auto arrow : arrows) {
        if (arrow->is_active()) {
            return true;
        }
    }
    return false;
}

void JProcessingTopology::set_active(bool active) {
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
        //   * The JProcessingController is requesting us to stop regardless of whether the topology finished or not

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

JProcessingTopology::JProcessingTopology(JApplication* app)
    : event_source_manager(app) {
}

JProcessingTopology::~JProcessingTopology() {
    for (auto evt_proc : event_processors) {
        delete evt_proc;
    }
    for (auto fac_gen : factory_generators) {
        delete fac_gen;
    }
    for (auto arrow : arrows) {
        delete arrow;
    }
    // TODO: Arrows and queues probably should be owned by JArrowProcessingController
}


