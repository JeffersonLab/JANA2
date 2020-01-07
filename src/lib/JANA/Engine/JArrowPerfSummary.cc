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

    os << "  +--------------------------+------------+--------+-----+---------+-------+--------+---------+-------------+" << std::endl;
    os << "  |           Name           |   Status   |  Type  | Par | Threads | Chunk | Thresh | Pending |  Completed  |" << std::endl;
    os << "  +--------------------------+------------+--------+-----+---------+-------+--------+---------+-------------+" << std::endl;

    for (auto as : s.arrows) {
        os << "  | "
           << std::setw(24) << std::left << as.arrow_name << " | "
           << std::setw(11) << as.status << "| "
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



