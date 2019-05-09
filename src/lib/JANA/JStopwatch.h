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

#ifndef JANA2_JTOPOLOGYMETRICS_H
#define JANA2_JTOPOLOGYMETRICS_H

#include <chrono>
#include <mutex>



class JStopwatch { // Coarse basic essential high-level

public:

    enum class Mode {Reset, Ticking, Stopped};

    struct Summary {

        size_t total_events_completed = 0;  // Since measuring started
        size_t latest_events_completed = 0; // Since previous measurement
        size_t thread_count = 0;
        double total_uptime_s = 0;
        double latest_uptime_s = 0;
        double avg_throughput_hz = 0;
        double latest_throughput_hz = 0;
    };

    void reset();

    void start(size_t current_event_count, size_t current_thread_count);

    void split(size_t current_event_count);

    void stop(size_t current_event_count);

    Summary summarize();

    // This is a hack needed because the relationship between
    // JProcessingTopology and JProcessingController is weird and bad.
    // Ideally, we want either:
    //   - stopwatch managed wholly by JPC, which means that JPC needs to be the observer instead of JPT
    //   - stopwatch managed wholly by JPT, who would have to know their own event count (only JPC does)
    void set_final_event_count(size_t event_count);

    Mode get_mode() { return mode_;};

private:

    using Clock = std::chrono::steady_clock;
    using secs = std::chrono::duration<double>;

    Mode mode_ = Mode::Reset;

    Clock::time_point start_time_ = Clock::now();
    Clock::time_point prev_time_ = Clock::now();
    Clock::time_point last_time_ = Clock::now();
    size_t start_event_count_ = 0;
    size_t prev_event_count_ = 0;
    size_t last_event_count_ = 0;
    size_t thread_count = 0;
    std::mutex mutex_;
};


#endif //JANA2_JTOPOLOGYMETRICS_H
