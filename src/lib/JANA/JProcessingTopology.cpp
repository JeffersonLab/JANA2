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
        // Someone told us to activate
        if (_stopwatch_status == StopwatchStatus::BeforeRun) {
            // We had not activated at all yet

            _stopwatch_status = StopwatchStatus::DuringRun;
            _start_time = jclock_t::now();
            _last_time = _start_time;
            for (auto arrow : sources) {
                // We activate our eventsources, which activate components downstream.
                arrow->set_active(true);
                arrow->notify_downstream(true);
            }
        }
        else if (_stopwatch_status == StopwatchStatus::AfterRun) {
            // TODO: We can clear and restart the timer, which we probably want to do on scale()
            throw JException("Topology is already stopped");
        }
    }
    else {
        // Someone has told us to deactivate. There are two ways to get here:
        //   * The last JEventProcessorArrow notifies us that he is deactivating because the topology is finished
        //   * The JProcessingController is requesting us to stop regardless of whether the topology finished or not

        if (_stopwatch_status == StopwatchStatus::DuringRun) {
            // We make our final timekeeping measurement, and we only do this once regardless of who calls us or why
            _stop_time = jclock_t::now();
            _stopwatch_status = StopwatchStatus::AfterRun;
        }

        // Make sure all arrows have been deactivated
        if (is_active()) {
            for (auto arrow : arrows) {
                arrow->set_active(false);
            }
        }
        _stopwatch.stop(0);  // HACK: We don't know how to compute our event count from here, so
                             // we do it later using set_final_event_count()
    }
}

JProcessingTopology::JProcessingTopology() : event_source_manager(japp) {
    //factoryset_pool = new JFactorySet
    // TODO: FactorySetPools appear to be nullptr... huh?
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
    for (auto queue : queues) {
        delete queue;
    }
    // TODO: Arrows and queues probably should be owned by JArrowProcessingController
}


