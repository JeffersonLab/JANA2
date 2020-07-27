
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JTOPOLOGYMETRICS_H
#define JANA2_JTOPOLOGYMETRICS_H

#include <chrono>
#include <mutex>
#include "JPerfSummary.h"

/// JPerfMetrics represents the highest-level metrics we can collect from a running JProcessingTopology.
/// Conceptually, it resembles a stopwatch, which is meant to be reset when the thread count changes,
/// started when processing starts, split every time a performance measurement should be made, and
/// stopped when processing finished. At each interaction the caller provides the current event count.
/// Timers are all managed internally.
class JPerfMetrics {

public:

    enum class Mode {Reset, Ticking, Stopped};

    Mode get_mode() { return mode_;};

    void reset();

    void start(size_t current_thread_count);

    void start(size_t current_event_count, size_t current_thread_count);

    void split(size_t current_event_count);

    void stop(size_t current_event_count);

    void stop();

    void summarize(JPerfSummary& summary);

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
    size_t thread_count_ = 0;
    std::mutex mutex_;
};


#endif //JANA2_JTOPOLOGYMETRICS_H
