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

#include "JArrowProcessingController.h"
#include "JCpuInfo.h"


void JArrowProcessingController::initialize() {
    _run_state = RunState::BeforeRun;
    _ncpus = JCpuInfo::GetNumCpus();
    _scheduler = new JScheduler(_topology->arrows, _ncpus);
}

void JArrowProcessingController::run(size_t nthreads) {

    scale(nthreads);
    _topology->set_active(true);
    _run_state = RunState::DuringRun;
    _start_time = jclock_t::now();
    _last_time = _start_time;
}

void JArrowProcessingController::scale(size_t nthreads) {

    size_t current_workers = _workers.size();
    while (current_workers < nthreads) {
        _workers.push_back(new JWorker(current_workers, _scheduler));
        current_workers++;
    }
    for (size_t i=0; i<nthreads; ++i) {
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

void JArrowProcessingController::request_stop() {
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
}

void JArrowProcessingController::wait_until_finished() {
}

void JArrowProcessingController::wait_until_stopped() {
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

size_t JArrowProcessingController::get_nthreads() {
    return _ncpus;
}

void JArrowProcessingController::measure_perf(JMetrics::TopologySummary& topology_perf) {

}

void JArrowProcessingController::measure_perf(JMetrics::TopologySummary& topology_perf,
                                         std::vector<JMetrics::ArrowSummary>& arrow_perf) {

}

void JArrowProcessingController::measure_perf(JMetrics::TopologySummary& topology_perf,
                                         std::vector<JMetrics::ArrowSummary>& arrow_perf,
                                         std::vector<JMetrics::WorkerSummary>& worker_perf) {

}

size_t JArrowProcessingController::get_nevents_processed() {
    for (JWorker* worker : _workers) {
        JMetrics::WorkerSummary summary;
        worker->measure_perf(summary);
    }
    uint64_t message_count = 0;
    for (JArrow* arrow : _topology->sinks) {
        message_count += arrow->get_metrics().get_total_message_count();
    }
    return message_count;
}

bool JArrowProcessingController::is_stopped() {
    for (JWorker* worker : _workers) {
        if (worker->get_runstate() != JWorker::RunState::Stopped) {
            return false;
        }
    }
    return true;
}

// TODO: Topology doesn't really distinguish between finished and stopped correctly, yet
//       Topology should remain active until finished, whereas stopped corresponds to Worker states
bool JArrowProcessingController::is_finished() {
    return !_topology->is_active();
}

JArrowProcessingController::~JArrowProcessingController() {
    request_stop();
    wait_until_stopped();
    for (JWorker* worker : _workers) {
        delete worker;
    }
}



