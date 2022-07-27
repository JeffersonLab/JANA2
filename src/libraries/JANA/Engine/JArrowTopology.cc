
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JArrowTopology.h"
#include "JEventProcessorArrow.h"
#include "JEventSourceArrow.h"


JArrowTopology::JArrowTopology() = default;

JArrowTopology::~JArrowTopology() {
    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto queue : queues) {
        delete queue;
    }
}

void JArrowTopology::on_status_change(Status old_status, Status new_status) {
    if (old_status == Status::Unopened && new_status == Status::Running) {
        for (auto source : sources) {
            // We activate all sources, and everything downstream activates automatically
            source->run();
        }
        metrics.start(0);
    }
    else if (old_status == Status::Running && new_status == Status::Stopped) {
        // TODO: I think we need to forcibly stop all arrows here.
        //       Complication 1: Probably want to block until all workers stop first.
        //       Complication 2: Probably want to stop all arrows that are still running
        metrics.stop();
    }
    else if (old_status == Status::Stopped && new_status == Status::Running) {
        metrics.reset();
        metrics.start(0);
    }
    else if (old_status == Status::Running && new_status == Status::Finished) {
        metrics.stop();
        // This used to call finalize() on _all_ arrows, but I don't think it should.
        // I think finalize() will propagate automatically. All the topology cares about
        // at this point is turning off the timer, I think.
    }
}
