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
    if (active && !is_active()) {
        for (auto arrow : sources) {
            arrow->set_active(true);
            arrow->notify_downstream(true);
        }
    }
    else {
        if (_run_state == RunState::DuringRun) {
            _stop_time = jclock_t::now();
            _run_state = RunState::AfterRun;
        }
    }
}

JProcessingTopology::JProcessingTopology() : event_source_manager(japp) {
    //factoryset_pool = new JFactorySet
    // TODO: FactorySetPools appear to be nullptr... huh?
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
    for (auto queue : queues) {
        delete queue;
    }
    // TODO: Arrows and queues probably should be owned by JArrowProcessingController
}


