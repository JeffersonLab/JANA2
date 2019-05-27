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
#include "JArrowPerfSummary.h"
#include "JCpuInfo.h"

#include <memory>

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

void JArrowProcessingController::initialize() {

    _scheduler = new JScheduler(_topology->arrows);
    japp->GetJParameterManager()->GetParameter("jana:pin_threads", _pin_to_cpu);
    japp->GetJParameterManager()->GetParameter("jana:cpu_mapping", _cpu_id_mapping);
    japp->GetJParameterManager()->GetParameter("jana:location_mapping", _location_id_mapping);
}

void JArrowProcessingController::run(size_t nthreads) {

    _topology->set_active(true);
    scale(nthreads);
}

void JArrowProcessingController::scale(size_t nthreads) {

    size_t next_worker_id = _workers.size();
    size_t cpu_map_count = _cpu_id_mapping.size();
    size_t loc_map_count = _location_id_mapping.size();

    while (next_worker_id < nthreads) {

        size_t next_cpu_id = (cpu_map_count == 0) ? next_worker_id : _cpu_id_mapping[next_worker_id % cpu_map_count];
        size_t next_loc_id = (loc_map_count == 0) ? 0 : _location_id_mapping[next_worker_id % loc_map_count];

        _workers.push_back(new JWorker(_scheduler, next_worker_id, next_cpu_id, next_loc_id, _pin_to_cpu));
        next_worker_id++;
    }

    for (size_t i=0; i<nthreads; ++i) {
        _workers.at(i)->start();
    };
    for (size_t i=nthreads; i<next_worker_id; ++i) {
        _workers.at(i)->request_stop();
    }
    for (size_t i=nthreads; i<next_worker_id; ++i) {
        _workers.at(i)->wait_for_stop();
    }
    _topology->metrics.reset();
    _topology->metrics.start(nthreads);
}

void JArrowProcessingController::request_stop() {
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
    // Tell the topology to stop timers and deactivate arrows
    _topology->set_active(false);
}

void JArrowProcessingController::wait_until_stopped() {
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
    for (JWorker* worker : _workers) {
        worker->wait_for_stop();
    }
}

bool JArrowProcessingController::is_stopped() {
    for (JWorker* worker : _workers) {
        if (worker->get_runstate() != JWorker::RunState::Stopped) {
            return false;
        }
    }
    // We have determined that all Workers have actually stopped
    return true;
}


JArrowProcessingController::~JArrowProcessingController() {
    request_stop();
    wait_until_stopped();
    for (JWorker* worker : _workers) {
        delete worker;
    }
}

void JArrowProcessingController::print_report() {
    auto metrics = measure_internal_performance();
    jout << *metrics;
}

void JArrowProcessingController::print_final_report() {
    print_report();
}

std::unique_ptr<const JArrowPerfSummary> JArrowProcessingController::measure_internal_performance() {

    // Measure perf on all Workers first, as this will prompt them to publish
    // any ArrowMetrics they have collected
    _perf_summary.workers.clear();
    for (JWorker* worker : _workers) {
        JMetrics::WorkerSummary summary;
        worker->measure_perf(summary);
        _perf_summary.workers.push_back(summary);
    }

    size_t monotonic_event_count = 0;
    for (JArrow* arrow : _topology->sinks) {
        monotonic_event_count += arrow->get_metrics().get_total_message_count();
    }

    // Uptime
    _topology->metrics.split(monotonic_event_count);
    _topology->metrics.summarize(_perf_summary);

    double worst_seq_latency = 0;
    double worst_par_latency = 0;

    _perf_summary.arrows.clear();
    for (JArrow* arrow : _topology->arrows) {
        JArrowMetrics::Status last_status;
        size_t total_message_count;
        size_t last_message_count;
        size_t total_queue_visits;
        size_t last_queue_visits;
        duration_t total_latency;
        duration_t last_latency;
        duration_t total_queue_latency;
        duration_t last_queue_latency;

        arrow->get_metrics().get(last_status, total_message_count, last_message_count, total_queue_visits,
                                 last_queue_visits, total_latency, last_latency, total_queue_latency, last_queue_latency);

        auto total_latency_ms = millisecs(total_latency).count();
        auto total_queue_latency_ms = millisecs(total_queue_latency).count();

        JMetrics::ArrowSummary summary;
        summary.arrow_type = arrow->get_type();
        summary.is_parallel = arrow->is_parallel();
        summary.is_active = arrow->is_active();
        summary.thread_count = arrow->get_thread_count();
        summary.arrow_name = arrow->get_name();
        summary.chunksize = arrow->get_chunksize();
        summary.messages_pending = arrow->get_pending();
        summary.is_upstream_active = !arrow->is_upstream_finished();
        summary.threshold = arrow->get_threshold();
        summary.status = arrow->get_status();

        summary.total_messages_completed = total_message_count;
        summary.last_messages_completed = last_message_count;
        summary.avg_latency_ms = total_latency_ms/total_message_count;
        summary.last_latency_ms = millisecs(last_latency).count()/last_message_count;
        summary.queue_visit_count = total_queue_visits;
        summary.avg_queue_latency_ms = total_queue_latency_ms / total_queue_visits;
        summary.avg_queue_overhead_frac = total_queue_latency_ms / (total_queue_latency_ms + total_latency_ms);

        if (arrow->is_parallel()) {
            worst_par_latency = std::max(worst_par_latency, summary.avg_latency_ms);
        } else {
            worst_seq_latency = std::max(worst_seq_latency, summary.avg_latency_ms);
        }
        _perf_summary.arrows.push_back(summary);
    }

    // bottlenecks
    _perf_summary.avg_seq_bottleneck_hz = 1e3 / worst_seq_latency;
    _perf_summary.avg_par_bottleneck_hz = 1e3 * _perf_summary.thread_count / worst_par_latency;

    auto tighter_bottleneck = std::min(_perf_summary.avg_seq_bottleneck_hz, _perf_summary.avg_par_bottleneck_hz);
    _perf_summary.avg_efficiency_frac = _perf_summary.avg_throughput_hz/tighter_bottleneck;

    return std::unique_ptr<JArrowPerfSummary>(new JArrowPerfSummary(_perf_summary));
}

std::unique_ptr<const JPerfSummary> JArrowProcessingController::measure_performance() {
    return measure_internal_performance();
}



