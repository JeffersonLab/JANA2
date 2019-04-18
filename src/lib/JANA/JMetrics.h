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

#ifndef JANA2_JMETRICS_H
#define JANA2_JMETRICS_H

namespace JMetrics {

struct TopologySummary {
    size_t messages_completed;
    double total_uptime_s;
    double last_uptime_s;
    double avg_throughput_hz;
    double last_throughput_hz;
    double avg_seq_bottleneck_hz;
    double avg_par_bottleneck_hz;
    double avg_efficiency_frac;

    double avg_useful_frac;
    double avg_queue_frac;
    double avg_scheduling_frac;
    double avg_retry_frac;
    double avg_idle_frac;
};

struct ArrowSummary {
    std::string arrow_name;
    bool is_parallel;
    bool is_active;
    bool is_upstream_active;
    bool has_backpressure;
    size_t thread_count;
    size_t messages_pending;
    size_t total_messages_completed;
    size_t last_messages_completed;
    size_t chunksize;
    double avg_latency_ms;
    double avg_queue_latency_ms;
    double last_latency_ms;
    double last_queue_latency_ms;
    size_t queue_visit_count;
};

struct WorkerSummary {
    int worker_id;
    int cpu_id;
    bool is_pinned;
    double avg_useful_time_ms;
    double avg_retry_time_ms;
    double avg_idle_time_ms;
    double avg_scheduler_time_ms;
    double last_useful_time_ms;
    double last_retry_time_ms;
    double last_idle_time_ms;
    double last_scheduler_time_ms;
    long scheduler_visit_count;
    std::string last_arrow_name;
    double last_arrow_avg_latency_ms;
    double last_arrow_avg_queue_latency_ms;
    double last_arrow_last_latency_ms;
    double last_arrow_last_queue_latency_ms;
    size_t last_arrow_queue_visit_count;
};


} // namespace JMetrics

#endif //JANA2_JMETRICS_H
