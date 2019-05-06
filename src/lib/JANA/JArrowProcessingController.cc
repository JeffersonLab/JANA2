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

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

void JArrowProcessingController::initialize() {
    _topology->_stopwatch_status = JProcessingTopology::StopwatchStatus::BeforeRun;
    _scheduler = new JScheduler(_topology->arrows);
}

void JArrowProcessingController::run(size_t nthreads) {

    scale(nthreads);
    _topology->set_active(true);
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
    _topology->_ncpus = nthreads;
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

size_t JArrowProcessingController::get_nthreads() {
    return _topology->_ncpus;
}

void JArrowProcessingController::measure_perf(JMetrics::TopologySummary& topology_summary) {

    std::vector<JMetrics::ArrowSummary> arrow_summary; // TODO: we could make these stateful to save allocations
    std::vector<JMetrics::WorkerSummary> worker_summary;
    measure_perf(topology_summary, arrow_summary, worker_summary);
}

void JArrowProcessingController::measure_perf(JMetrics::TopologySummary& topology_summary,
                                              std::vector<JMetrics::ArrowSummary>& arrow_summary) {

    std::vector<JMetrics::WorkerSummary> worker_summary;
    measure_perf(topology_summary, arrow_summary, worker_summary);
}

void JArrowProcessingController::measure_perf(JMetrics::TopologySummary& topology_perf,
                                         std::vector<JMetrics::ArrowSummary>& arrow_perf,
                                         std::vector<JMetrics::WorkerSummary>& worker_perf) {

    worker_perf.clear();
    for (JWorker* worker : _workers) {
        JMetrics::WorkerSummary summary;
        worker->measure_perf(summary);
        worker_perf.push_back(summary);
    }

    double worst_seq_latency = 0;
    double worst_par_latency = 0;
    arrow_perf.clear();
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
        summary.is_active = arrow->is_active();
        summary.thread_count = arrow->get_thread_count();
        summary.arrow_name = arrow->get_name();
        summary.chunksize = arrow->get_chunksize();
        summary.is_parallel = arrow->is_parallel();
        summary.is_upstream_active = !arrow->is_upstream_finished();
        summary.total_messages_completed = total_message_count;
        summary.last_messages_completed = last_message_count;
        summary.avg_latency_ms = total_latency_ms/total_message_count;
        summary.last_latency_ms = millisecs(last_latency).count()/last_message_count;
        summary.messages_pending = 0; // TODO: Get this from arrow eventually
        summary.queue_visit_count = total_queue_visits;
        summary.avg_queue_latency_ms = total_queue_latency_ms / total_queue_visits;
        summary.avg_queue_overhead_frac = total_queue_latency_ms / (total_queue_latency_ms + total_latency_ms);

        if (arrow->is_parallel()) {
            worst_par_latency = std::max(worst_par_latency, summary.avg_latency_ms);
        } else {
            worst_seq_latency = std::max(worst_seq_latency, summary.avg_latency_ms);
        }
        arrow_perf.push_back(summary);
    }

    // Messages completed
    topology_perf.messages_completed = 0;
    for (JArrow* arrow : _topology->sinks) {
        topology_perf.messages_completed += arrow->get_metrics().get_total_message_count();
    }
    uint64_t message_delta = topology_perf.messages_completed - _topology->_last_message_count;

    // Uptime
    if (_topology->_stopwatch_status == JProcessingTopology::StopwatchStatus::AfterRun) {
        topology_perf.last_uptime_s = secs(_topology->_stop_time - _topology->_last_time).count();
        topology_perf.total_uptime_s = secs(_topology->_stop_time - _topology->_start_time).count();
    }
    else { // StopwatchStatus::DuringRun
        auto current_time = jclock_t::now();
        topology_perf.last_uptime_s = secs(current_time - _topology->_last_time).count();
        topology_perf.total_uptime_s = secs(current_time - _topology->_start_time).count();
        _topology->_last_time = current_time;
        _topology->_last_message_count = topology_perf.messages_completed;
    }

    // Throughput
    topology_perf.avg_throughput_hz = topology_perf.messages_completed / topology_perf.total_uptime_s;
    topology_perf.last_throughput_hz = message_delta / topology_perf.last_uptime_s;

    // bottlenecks
    topology_perf.avg_seq_bottleneck_hz = 1e3 / worst_seq_latency;
    topology_perf.avg_par_bottleneck_hz = 1e3 * _topology->_ncpus / worst_par_latency;

    auto tighter_bottleneck = std::min(topology_perf.avg_seq_bottleneck_hz, topology_perf.avg_par_bottleneck_hz);
    topology_perf.avg_efficiency_frac = topology_perf.avg_throughput_hz/tighter_bottleneck;

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
    JMetrics::TopologySummary s;
    std::vector<JMetrics::ArrowSummary> arrow_summary; // TODO: we could make these stateful to save allocations
    std::vector<JMetrics::WorkerSummary> worker_summary;

    measure_perf(s, arrow_summary, worker_summary);

    std::ostringstream os;
    if (_topology->_stopwatch_status == JProcessingTopology::StopwatchStatus::BeforeRun) {
        os << "Nothing running!" << std::endl;
    }
    else {
        os << std::endl;
        os << " TOPOLOGY STATUS" << std::endl;
        os << " ---------------" << std::endl;
        os << " Thread team size [count]:    " << _topology->_ncpus << std::endl;
        os << " Total uptime [s]:            " << std::setprecision(4) << s.total_uptime_s << std::endl;
        os << " Uptime delta [s]:            " << std::setprecision(4) << s.last_uptime_s << std::endl;
        os << " Completed events [count]:    " << s.messages_completed << std::endl;
        os << " Inst throughput [Hz]:        " << std::setprecision(3) << s.last_throughput_hz << std::endl;
        os << " Avg throughput [Hz]:         " << std::setprecision(3) << s.avg_throughput_hz << std::endl;
        os << " Sequential bottleneck [Hz]:  " << std::setprecision(3) << s.avg_seq_bottleneck_hz << std::endl;
        os << " Parallel bottleneck [Hz]:    " << std::setprecision(3) << s.avg_par_bottleneck_hz << std::endl;
        os << " Efficiency [0..1]:           " << std::setprecision(3) << s.avg_efficiency_frac << std::endl;
        os << std::endl;
        os << " +--------------------------+-----+-----+---------+-------+-------------+" << std::endl;
        os << " |           Name           | Par | Act | Threads | Chunk |  Completed  |" << std::endl;
        os << " +--------------------------+-----+-----+---------+-------+-------------+" << std::endl;
        for (auto as : arrow_summary) {
            os << " | "
               << std::setw(24) << std::left << as.arrow_name << " | "
               << std::setw(3) << std::right << (as.is_parallel ? " T " : " F ") << " | "
               << std::setw(3) << (as.is_active ? " T " : " F ") << " | "
               << std::setw(7) << as.thread_count << " |"
               << std::setw(6) << as.chunksize << " |"
               << std::setw(12) << as.total_messages_completed << " |"
               << std::endl;
        }
        os << " +--------------------------+-----+-----+---------+-------+-------------+" << std::endl;


        os << " +--------------------------+-------------+--------------+----------------+--------------+----------------+" << std::endl;
        os << " |           Name           | Avg latency | Inst latency | Queue latency  | Queue visits | Queue overhead | " << std::endl;
        os << " |                          | [ms/event]  |  [ms/event]  |   [ms/visit]   |    [count]   |     [0..1]     | " << std::endl;
        os << " +--------------------------+-------------+--------------+----------------+--------------+----------------+" << std::endl;

        for (auto as : arrow_summary) {
            os << " | " << std::setprecision(3)
               << std::setw(24) << std::left << as.arrow_name << " | "
               << std::setw(11) << std::right << as.avg_latency_ms << " |"
               << std::setw(13) << as.last_latency_ms << " |"
               << std::setw(15) << as.avg_queue_latency_ms << " |"
               << std::setw(13) << as.queue_visit_count << " |"
               << std::setw(15) << as.avg_queue_overhead_frac << " |"
               << std::endl;
        }
        os << " +--------------------------+-------------+--------------+----------------+--------------+----------------+" << std::endl;


        os << " +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;
        os << " | ID | Last arrow name      | Useful time | Retry time | Idle time | Scheduler time | Scheduler visits |" << std::endl;
        os << " |    |                      |     [ms]    |    [ms]    |    [ms]   |      [ms]      |     [count]      |" << std::endl;
        os << " +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;

        for (auto ws : worker_summary) {
            os << " |"
               << std::setw(3) << std::right << ws.worker_id << " | "
               << std::setw(20) << std::left << ws.last_arrow_name << " |"
               << std::setw(12) << std::right << ws.last_useful_time_ms << " |"
               << std::setw(11) << ws.last_retry_time_ms << " |"
               << std::setw(10) << ws.last_idle_time_ms << " |"
               << std::setw(15) << ws.last_scheduler_time_ms << " |"
               << std::setw(17) << ws.scheduler_visit_count << " |"
               << std::endl;
        }
        os << " +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;
    }

    jout << os.str();

}

void JArrowProcessingController::print_final_report() {
    print_report();
}



