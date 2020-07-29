
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JPERFORMANCESUMMARY_H
#define JANA2_JPERFORMANCESUMMARY_H


#include <cstddef>
#include <ostream>
#include <iomanip>

/// JPerfSummary is a plain-old-data container for performance metrics.
/// JProcessingControllers expose a JPerfSummary object, which they may
/// extend in order to expose additional, implementation-specific information.
struct JPerfSummary {

    size_t monotonic_events_completed = 0;  // Since program started
    size_t total_events_completed = 0;      // Since run or rescale started
    size_t latest_events_completed = 0;     // Since previous measurement
    size_t thread_count = 0;
    double total_uptime_s = 0;
    double latest_uptime_s = 0;
    double avg_throughput_hz = 0;
    double latest_throughput_hz = 0;

    JPerfSummary() = default;
    JPerfSummary(const JPerfSummary& other) = default;
    virtual ~JPerfSummary() = default;
};

inline std::ostream& operator<<(std::ostream& os, const JPerfSummary& x) {
    os << "  Threads:               " << x.thread_count << std::endl
       << "  Events processed:      " << x.total_events_completed << std::endl
       << "  Inst throughput [Hz]:  " << x.latest_throughput_hz << std::endl
       << "  Avg throughput [Hz]:   " << x.avg_throughput_hz << std::endl;
    return os;
}



#endif //JANA2_JPERFORMANCESUMMARY_H
