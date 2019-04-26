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

