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

#include "JPerfMetrics.h"
#include "JException.h"


/// Reset is meant to be called between separate run()s or scale()s.
/// It sets everything in the metric back to zero or now(), except the start_event_count,
/// which increases monotonically throughout the life of the program.
void JPerfMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = Clock::now();
    mode_ = Mode::Reset;
    start_time_ = now;
    prev_time_ = now;
    last_time_ = now;
    start_event_count_ = last_event_count_;
    prev_event_count_ = last_event_count_;
    last_event_count_ = 0;
    thread_count_ = 0;
}

/// This implementation of start() assumes that no processing has
/// occurred while the stopwatch wasn't running. Thus it can reuse
/// the event count from the previous run. This is convenient
/// because that value can be annoying to calculate.
void JPerfMetrics::start(size_t current_thread_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = Clock::now();
    if (mode_ == Mode::Reset) {
        mode_ = Mode::Ticking;
        start_time_ = now;
        last_time_ = now;
        prev_time_ = now;
        thread_count_ = current_thread_count;
    }
    else {
        throw JException("Stopwatch must be reset before it can be started!");
    }
}

/// This is the preferred implementation of start(), which doesn't make
/// dangerous assumptions about nothing happening in the meantime
void JPerfMetrics::start(size_t current_event_count, size_t current_thread_count) {

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
        thread_count_ = current_thread_count;
    }
    else {
        throw JException("Stopwatch must be reset before it can be started!");
    }
}


/// Split accumulates another (monotonic) event count measurement.

/// If the stopwatch has already been stopped, split will
/// update the event count without updating the time.
/// This is convenient because we sometimes don't have access
/// to the current event count but still wish to stop the timer,
/// but it is dangerous because it makes the caller responsible for ensuring
/// that further events don't get processed after the stopwatch stops.
void JPerfMetrics::split(size_t current_event_count) {

    std::lock_guard<std::mutex> lock(mutex_);
    if (mode_ == Mode::Ticking) {
        prev_time_ = last_time_;
        last_time_ = Clock::now();
        prev_event_count_ = last_event_count_;
        last_event_count_ = current_event_count;
    }
    else {
        // TODO: Reconsider this
        last_event_count_ = current_event_count;
    }
}


/// Stop without recording a final measurement. This can be provided
/// in a later call to split(). `void stop(current_event_count)` is preferred.
void JPerfMetrics::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = Clock::now();
    if (mode_ == Mode::Ticking) {
        mode_ = Mode::Stopped;
        prev_time_ = last_time_;
        last_time_ = now;
    }
}

/// Stop, and record the final measurement at that time. This should be used
/// instead of `void stop()` whenever possible.
void JPerfMetrics::stop(size_t current_event_count) {

    std::lock_guard<std::mutex> lock(mutex_);
    auto now = Clock::now();
    if (mode_ == Mode::Ticking) {
        mode_ = Mode::Stopped;
        prev_time_ = last_time_;
        last_time_ = now;
        prev_event_count_ = last_event_count_;
        last_event_count_ = current_event_count;
    }
}

void JPerfMetrics::summarize(JPerfSummary& summary) {

    std::lock_guard<std::mutex> lock(mutex_);
    summary.monotonic_events_completed = last_event_count_;
    summary.total_events_completed = last_event_count_ - start_event_count_;
    summary.latest_events_completed = last_event_count_ - prev_event_count_;
    summary.total_uptime_s  = secs(last_time_ - start_time_).count();
    summary.latest_uptime_s = secs(last_time_ - prev_time_).count();
    summary.avg_throughput_hz = summary.total_events_completed / summary.total_uptime_s;
    summary.latest_throughput_hz = summary.latest_events_completed / summary.latest_uptime_s;
    summary.thread_count = thread_count_;
}



