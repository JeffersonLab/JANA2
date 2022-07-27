
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JArrowTopology.h"
#include "JEventProcessorArrow.h"
#include "JEventSourceArrow.h"


JArrowTopology::JArrowTopology() = default;

JArrowTopology::~JArrowTopology() {
    finish();  // In case we stopped() but didn't finish(),
    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto queue : queues) {
        delete queue;
    }
}

void JArrowTopology::drain() {
    for (auto source : sources) {
        source->finish();
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
        // This stops all arrows WITHOUT draining queues.
        // There might still be some events being worked on, so the caller to stop() should call wait_until_stopped() afterwards.
        // Note that this won't call finish(), but we are allowed to call finish() later (importantly, after wait_until_stopped)
        for (auto arrow: arrows) {
            arrow->stop(); // If arrow is not running, stop() is a no-op
        }
        metrics.stop();
    }
    else if (old_status == Status::Stopped && new_status == Status::Running) {
        metrics.reset();
        metrics.start(0);
        for (auto source: sources) {
            if (source->get_status() != JActivable::Status::Finished) {
                source->run();
            }
        }
    }
    else if (new_status == Status::Finished) {
        metrics.stop();
    }
}
