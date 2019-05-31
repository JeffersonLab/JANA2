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

#include "JProcessorMapping.h"
#include <iomanip>

void JProcessorMapping::initialize(AffinityStrategy affinity, LocalityStrategy locality) {

    m_affinity_strategy = affinity;
    m_locality_strategy = locality;

    // Capture lscpu info
    // Parse into table

    // Figure out location
    for (Row& row : m_mapping) {
        switch (m_locality_strategy) {
            case LocalityStrategy::Error:
            case LocalityStrategy::Global:          row.location_id = 0; break;
            case LocalityStrategy::CpuLocal:        row.location_id = row.cpu_id; break;
            case LocalityStrategy::CoreLocal:       row.location_id = row.core_id; break;
            case LocalityStrategy::NumaDomainLocal: row.location_id = row.numa_domain_id; break;
            case LocalityStrategy::SocketLocal:     row.location_id = row.socket_id; break;
        }
    }

    switch (m_affinity_strategy) {
        case AffinityStrategy::None: break;
        case AffinityStrategy::ComputeBound: break;
        case AffinityStrategy::MemoryBound: break;
    }
    // Sort according to strategy
}


std::ostream& operator<<(std::ostream& os, const JProcessorMapping& m) {

    os << "+--------+----------+-------+--------+-----------+--------+" << std::endl
       << "| worker | location |  cpu  |  core  | numa node | socket |" << std::endl
       << "+--------+----------+-------+--------+-----------+--------+" << std::endl;

    size_t worker_id = 0;
    for (const JProcessorMapping::Row& row : m.m_mapping) {
        os <<  "| " << std::right << std::setw(6) << worker_id++;
        os << " | " << std::setw(8) << row.location_id;
        os << " | " << std::setw(3) << row.cpu_id;
        os << " | " << std::setw(4) << row.core_id;
        os << " | " << std::setw(9) << row.numa_domain_id;
        os << " | " << std::setw(6) << row.socket_id << " |" << std::endl;
    }

    os << "+---------+----------+-------+--------+-----------+--------+" << std::endl;

}
