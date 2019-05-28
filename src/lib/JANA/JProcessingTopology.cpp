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
            }
        }
        else {
            // Arrows have all deactivated: Stop timer
            metrics.stop();
            status = Status::Inactive;
            // TODO: Distinguish deactivation due to finishing from deactivation due to request_stop()
        }
    }
}

JProcessingTopology::JProcessingTopology()
    : event_source_manager(japp) {
}

JProcessingTopology::~JProcessingTopology() {
    for (auto evt_proc : event_processors) {
        evt_proc->Finish(); // TODO: Arrow should do this before deactivating
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


