//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#include "JProcessingController.h"
#include "JCpuInfo.h"


void JProcessingController::initialize() {
    // Set exit code
    // Set _quitting, _draining_queues

    _run_state = RunState::BeforeRun;
    _ncpus = JCpuInfo::GetNumCpus();
}

void JProcessingController::run(size_t nthreads) {

    scale(nthreads);
    _activator.set_active(true);
    _run_state = RunState::DuringRun;
    _start_time = jclock_t::now();
    _last_time = _start_time;
}

void JProcessingController::scale(size_t nthreads) {

    if (_schedulers.empty()) {
        _schedulers.push_back(new JScheduler(_arrows, _ncpus));
    }

    size_t current_workers = _workers.size();
    while (current_workers < nthreads) {
        _workers.push_back(new JWorker(current_workers, _schedulers[0]));
        current_workers++;
    }
    for (int i=0; i<nthreads; ++i) {
        _workers.at(i)->start();
    };
    for (size_t i=nthreads; i<current_workers; ++i) {
        _workers.at(i)->request_stop();
    }
    for (size_t i=nthreads; i<current_workers; ++i) {
        _workers.at(i)->wait_for_stop();
    }
    _ncpus = nthreads;
}

void JProcessingController::request_stop() {
    for (JArrow* arrow : _arrows) {
        arrow->set_active(false);
    }
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
}

void JProcessingController::wait_until_finished() {
}

void JProcessingController::wait_until_stopped() {
    for (JWorker* worker : _workers) {
        worker->wait_for_stop();
    }
    if (_run_state == RunState::DuringRun) {
        // We shouldn't usually end up here because sinks notify us when
        // they deactivate, automatically calling JTopology::set_active(false),
        // which stops the clock as soon as possible.
        // However, if for unknown reasons nobody notifies us, we still want to change
        // run state in an orderly fashion. If we do end up here, though, our _stop_time
        // will be late, throwing our metrics off.
        _stop_time = jclock_t::now();
    }
    _run_state = RunState::AfterRun;
}

size_t JProcessingController::get_nthreads() {
    return 0;
}

void JProcessingController::measure_perf(JMetrics::TopologySummary& topology_perf) {

}

void JProcessingController::measure_perf(JMetrics::TopologySummary& topology_perf,
                                         std::vector<JMetrics::ArrowSummary>& arrow_perf) {

}

void JProcessingController::measure_perf(JMetrics::TopologySummary& topology_perf,
                                         std::vector<JMetrics::ArrowSummary>& arrow_perf,
                                         std::vector<JMetrics::WorkerSummary>& worker_perf) {

}

size_t JProcessingController::get_nevents_processed() {
    uint64_t message_count = 0;
    for (JArrow* arrow : _sinks) {
        message_count += arrow->get_metrics().get_total_message_count();
    }
    return message_count;
}

bool JProcessingController::is_stopped() {
    for (JWorker* worker : _workers) {
        if (worker->get_runstate() != JWorker::RunState::Stopped) {
            return false;
        }
    }
    return true;
}

// TODO: Topology doesn't really distinguish between finished and stopped correctly, yet
//       Topology should remain active until finished, whereas stopped corresponds to Worker states
bool JProcessingController::is_finished() {
    return !_activator.is_active();
}

bool JProcessingController::TopologyActivator::is_active() {
    for (auto arrow : _arrows) {
        if (arrow->is_active()) {
            return true;
        }
    }
    return false;
}

void JProcessingController::TopologyActivator::set_active(bool is_active) {
    if (active && !is_active()) {
        for (auto arrow : _sources) {
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

class JThreadManager;
JThreadManager* JProcessingController::GetJThreadManager() const {
    return nullptr;
}

template<typename T> class JTask;
std::shared_ptr<JTask<void>> JProcessingController::GetVoidTask() {
    throw;
}

