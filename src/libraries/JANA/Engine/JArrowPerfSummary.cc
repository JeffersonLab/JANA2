
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JArrowPerfSummary.h"

#include <ostream>
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const JArrowPerfSummary& s) {

    os << std::endl;
    os << "  Thread team size [count]:    " << s.thread_count << std::endl;
    os << "  Total uptime [s]:            " << std::setprecision(4) << s.total_uptime_s << std::endl;
    os << "  Uptime delta [s]:            " << std::setprecision(4) << s.latest_uptime_s << std::endl;
    os << "  Completed events [count]:    " << s.total_events_completed << std::endl;
    os << "  Inst throughput [Hz]:        " << std::setprecision(3) << s.latest_throughput_hz << std::endl;
    os << "  Avg throughput [Hz]:         " << std::setprecision(3) << s.avg_throughput_hz << std::endl;
    os << "  Sequential bottleneck [Hz]:  " << std::setprecision(3) << s.avg_seq_bottleneck_hz << std::endl;
    os << "  Parallel bottleneck [Hz]:    " << std::setprecision(3) << s.avg_par_bottleneck_hz << std::endl;
    os << "  Efficiency [0..1]:           " << std::setprecision(3) << s.avg_efficiency_frac << std::endl;
    os << std::endl;

    os << "  +--------------------------+--------+-----+---------+-------+--------+---------+-------------+" << std::endl;
    os << "  |           Name           |  Type  | Par | Threads | Chunk | Thresh | Pending |  Completed  |" << std::endl;
    os << "  +--------------------------+--------+-----+---------+-------+--------+---------+-------------+" << std::endl;

    for (auto as : s.arrows) {
        os << "  | "
           << std::setw(24) << std::left << as.arrow_name << " | "
           << std::setw(6) << std::left << as.arrow_type << " | "
           << std::setw(3) << std::right << (as.is_parallel ? " T " : " F ") << " | "
           << std::setw(7) << as.thread_count << " |"
           << std::setw(6) << as.chunksize << " |";

        if (as.arrow_type != JArrow::NodeType::Source) {

            os << std::setw(7) << as.threshold << " |"
               << std::setw(8) << as.messages_pending << " |";
        }
        else {

            os << "      - |       - |";
        }
        os << std::setw(12) << as.total_messages_completed << " |"
           << std::endl;
    }
    os << "  +--------------------------+------------+--------+-----+---------+-------+--------+---------+-------------+" << std::endl;


    os << "  +--------------------------+-------------+--------------+----------------+--------------+----------------+" << std::endl;
    os << "  |           Name           | Avg latency | Inst latency | Queue latency  | Queue visits | Queue overhead | " << std::endl;
    os << "  |                          | [ms/event]  |  [ms/event]  |   [ms/visit]   |    [count]   |     [0..1]     | " << std::endl;
    os << "  +--------------------------+-------------+--------------+----------------+--------------+----------------+" << std::endl;

    for (auto as : s.arrows) {
        os << "  | " << std::setprecision(3)
           << std::setw(24) << std::left << as.arrow_name << " | "
           << std::setw(11) << std::right << as.avg_latency_ms << " |"
           << std::setw(13) << as.last_latency_ms << " |"
           << std::setw(15) << as.avg_queue_latency_ms << " |"
           << std::setw(13) << as.queue_visit_count << " |"
           << std::setw(15) << as.avg_queue_overhead_frac << " |"
           << std::endl;
    }
    os << "  +--------------------------+-------------+--------------+----------------+--------------+----------------+" << std::endl;


    os << "  +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;
    os << "  | ID | Last arrow name      | Useful time | Retry time | Idle time | Scheduler time | Scheduler visits |" << std::endl;
    os << "  |    |                      |     [ms]    |    [ms]    |    [ms]   |      [ms]      |     [count]      |" << std::endl;
    os << "  +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;

    for (auto ws : s.workers) {
        os << "  |"
           << std::setw(3) << std::right << ws.worker_id << " | "
           << std::setw(20) << std::left << ws.last_arrow_name << " |"
           << std::setw(12) << std::right << ws.last_useful_time_ms << " |"
           << std::setw(11) << ws.last_retry_time_ms << " |"
           << std::setw(10) << ws.last_idle_time_ms << " |"
           << std::setw(15) << ws.last_scheduler_time_ms << " |"
           << std::setw(17) << ws.scheduler_visit_count << " |"
           << std::endl;
    }
    os << "  +----+----------------------+-------------+------------+-----------+----------------+------------------+" << std::endl;
    return os;
}



