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

#include "JStopwatch.h"
#include "JException.h"


void JStopwatch::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    mode_ = Mode::Reset;
}

void JStopwatch::start(size_t current_event_count, size_t current_thread_count) {

    std::lock_guard<std::mutex> lock(mutex_);
    auto now = Clock::now();
    if (mode_ == Mode::Reset) {
        mode_ = Mode::Ticking;
        start_time_ = now;
        last_time_ = now;
        prev_time_ = now;
        start_event_count_ = current_event_count;
        prev_event_count_ = current_event_count;
        last_event_count_ = current_event_count;
        thread_count = current_thread_count;
    }
    else {
        throw JException("This is not how you use a stopwatch!");
    }
}

void JStopwatch::split(size_t current_event_count) {

    std::lock_guard<std::mutex> lock(mutex_);
    if (mode_ == Mode::Ticking) {
        prev_time_ = last_time_;
        last_time_ = Clock::now();
    }
    else {
        throw JException("Stopwatch is not ticking!");
    }
    if (mode_ != Mode::Reset) {
        prev_event_count_ = last_event_count_;
        last_event_count_ = current_event_count;
    }
}

void JStopwatch::stop(size_t current_event_count) {

    std::lock_guard<std::mutex> lock(mutex_);
    auto now = Clock::now();
    if (mode_ != Mode::Reset) {
        mode_ = Mode::Stopped;
        prev_time_ = last_time_;
        last_time_ = now;
        prev_event_count_ = last_event_count_;
        last_event_count_ = current_event_count;
    }
    else {
        throw JException("This is not how you use a stopwatch!");
    }
}

JStopwatch::Summary JStopwatch::summarize() {

    std::lock_guard<std::mutex> lock(mutex_);
    if (mode_ == Mode::Reset) {
        throw JException("Stopwatch never ran!");
    }
    Summary summary;
    summary.total_events_completed = last_event_count_ - start_event_count_;
    summary.latest_events_completed = last_event_count_ - prev_event_count_;
    summary.total_uptime_s  = secs(last_time_ - start_time_).count();
    summary.latest_uptime_s = secs(last_time_ - prev_time_).count();
    summary.avg_throughput_hz = summary.total_events_completed / summary.total_uptime_s;
    summary.latest_throughput_hz = summary.latest_events_completed / summary.latest_uptime_s;
    return summary;
}

void JStopwatch::set_final_event_count(size_t event_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mode_ != Mode::Stopped) {
        throw JException("Can only set the final event count on a stopwatch which has stopped!");
    }
    last_event_count_ = event_count;
}



