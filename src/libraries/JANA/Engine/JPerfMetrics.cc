
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JPerfMetrics.h"
#include <JANA/JException.h>


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



