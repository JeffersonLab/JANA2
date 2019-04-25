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

#ifndef JANA2_JPROCESSINGCONTROLLER_H
#define JANA2_JPROCESSINGCONTROLLER_H

#include <vector>

#include <JANA/JArrow.h>
#include <JANA/JWorker.h>
#include <JANA/JMetrics.h>


class JProcessingController {
public:
    JProcessingController(JProcessingTopology* topology) : _topology(topology) {};

    void initialize();
    void run(size_t nthreads);
    void scale(size_t nthreads);
    void request_stop();
    void wait_until_finished();
    void wait_until_stopped();

    bool is_stopped();
    bool is_finished();

    size_t get_nthreads();
    size_t get_nevents_processed();


    void measure_perf(JMetrics::TopologySummary& topology_perf);

    void measure_perf(JMetrics::TopologySummary& topology_perf,
                      std::vector<JMetrics::ArrowSummary>& arrow_perf);

    void measure_perf(JMetrics::TopologySummary& topology_perf,
                      std::vector<JMetrics::ArrowSummary>& arrow_perf,
                      std::vector<JMetrics::WorkerSummary>& worker_perf);



private:

    using jclock_t = std::chrono::steady_clock;
    enum class RunState { BeforeRun, DuringRun, AfterRun };

    class TopologyActivator : public JActivable {
    public:
        bool is_active() override;
        void set_active(bool is_active) override;
    };

    JProcessingTopology* _topology;  // TODO: Move a lot of the things below into here

    std::vector<JArrow*> _arrows;
    std::vector<QueueBase*> _queues;
    std::vector<JWorker*> _workers;         // One per cpu
    std::vector<JScheduler*> _schedulers;   // One per NUMA domain
    // TODO: How much NUMA stuff lives in ProcessingController vs TopologyBuilder?

    std::vector<JArrow*> _sources;          // Sources needed for activation
    std::vector<JArrow*> _sinks;            // Sinks needed for finished message count

    JLogger _logger;

    RunState _run_state = RunState::BeforeRun;
    TopologyActivator _activator;
    jclock_t::time_point _start_time;
    jclock_t::time_point _last_time;
    jclock_t::time_point _stop_time;
    size_t _last_message_count = 0;
    uint32_t _ncpus;



};

#endif //JANA2_JPROCESSINGCONTROLLER_H

