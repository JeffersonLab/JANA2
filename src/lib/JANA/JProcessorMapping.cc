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
#include <unistd.h>
#include <algorithm>

void JProcessorMapping::initialize(AffinityStrategy affinity, LocalityStrategy locality) {

    m_affinity_strategy = affinity;
    m_locality_strategy = locality;

    // Capture lscpu info

    int pipe_fd[2]; // We want to pipe lscpu's stdout straight to us
    if (pipe(pipe_fd) == -1) {
        m_error_msg = "Unable to open pipe";
        return;
    }
    pid_t pid = fork();
    if (pid == -1) { // Failure
        m_error_msg = "Unable to fork";
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    }
    else if (pid == 0) { // We are the child process
        dup2(pipe_fd[1], 1);  // Redirect stdout to pipe
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execlp("lscpu", "lscpu", "-b", "-pcpu,core,node,socket", nullptr);
        exit(-1);
    }
    else { // We are the parent process
        close(pipe_fd[1]);
    }

    // In case initialize() is called multiple times, we don't want old data to interfere
    m_loc_count = 1;
    m_mapping.clear();

    // Parse into table
    FILE* infile = fdopen(pipe_fd[0], "r");

    const size_t buffer_size = 300;
    char buffer[buffer_size];

    while (fgets(buffer, buffer_size, infile) != nullptr) {
        if (buffer[0] == '#') continue;
        Row row;
        int count = sscanf(buffer, "%zu,%zu,%zu,%zu", &row.cpu_id, &row.core_id, &row.numa_domain_id, &row.socket_id);
        if (count == 4) {
            switch (m_locality_strategy) {
                case LocalityStrategy::CpuLocal:        row.location_id = row.cpu_id; break;
                case LocalityStrategy::CoreLocal:       row.location_id = row.core_id; break;
                case LocalityStrategy::NumaDomainLocal: row.location_id = row.numa_domain_id; break;
                case LocalityStrategy::SocketLocal:     row.location_id = row.socket_id; break;
                case LocalityStrategy::Global:
                default:                                row.location_id = 0; break;
            }
            if (row.location_id >= m_loc_count) {
                // Assume all of these ids are zero-indexed and contiguous
                m_loc_count = row.location_id + 1;
            }
            m_mapping.push_back(row);
        }
    }
    fclose(infile);

    if (m_mapping.empty()) {
        m_error_msg = "Unable to parse lscpu output";
        return;
    }

    // Apply affinity strategy by sorting over sets of columns
    switch (m_affinity_strategy) {


        case AffinityStrategy::ComputeBound:

            std::stable_sort(m_mapping.begin(), m_mapping.end(),
                             [](const Row& lhs, const Row& rhs) -> bool { return lhs.cpu_id < rhs.cpu_id; });
            break;

        case AffinityStrategy::MemoryBound:

            std::stable_sort(m_mapping.begin(), m_mapping.end(),
                             [](const Row& lhs, const Row& rhs) -> bool { return lhs.cpu_id < rhs.cpu_id; });

            std::stable_sort(m_mapping.begin(), m_mapping.end(),
                             [](const Row& lhs, const Row& rhs) -> bool { return lhs.numa_domain_id < rhs.numa_domain_id; });
            break;

        default:
            break;
    }

    // Apparently we were successful
    m_initialized = true;
}

std::ostream& operator<<(std::ostream& os, const JProcessorMapping::AffinityStrategy& s) {
    switch (s) {
        case JProcessorMapping::AffinityStrategy::ComputeBound: os << "compute-bound (favor fewer hyperthreads)"; break;
        case JProcessorMapping::AffinityStrategy::MemoryBound: os << "memory-bound (favor fewer NUMA domains)"; break;
        case JProcessorMapping::AffinityStrategy::None: os << "none"; break;
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, const JProcessorMapping::LocalityStrategy& s) {
    switch (s) {
        case JProcessorMapping::LocalityStrategy::CpuLocal: os << "cpu-local"; break;
        case JProcessorMapping::LocalityStrategy::CoreLocal: os << "core-local"; break;
        case JProcessorMapping::LocalityStrategy::NumaDomainLocal: os << "numa-domain-local"; break;
        case JProcessorMapping::LocalityStrategy::SocketLocal: os << "socket-local"; break;
        case JProcessorMapping::LocalityStrategy::Global: os << "global"; break;
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, const JProcessorMapping& m) {

    os << "Affinity strategy: " << m.m_affinity_strategy << std::endl;
    os << "Locality strategy: " << m.m_locality_strategy << std::endl;
    os << "Location count: " << m.m_loc_count << std::endl;

    if (m.m_initialized) {
        os << "+--------+----------+-------+--------+-----------+--------+" << std::endl
           << "| worker | location |  cpu  |  core  | numa node | socket |" << std::endl
           << "+--------+----------+-------+--------+-----------+--------+" << std::endl;

        size_t worker_id = 0;
        for (const JProcessorMapping::Row& row : m.m_mapping) {
            os <<  "| " << std::right << std::setw(6) << worker_id++;
            os << " | " << std::setw(8) << row.location_id;
            os << " | " << std::setw(5) << row.cpu_id;
            os << " | " << std::setw(6) << row.core_id;
            os << " | " << std::setw(9) << row.numa_domain_id;
            os << " | " << std::setw(6) << row.socket_id << " |" << std::endl;
        }

        os << "+--------+----------+-------+--------+-----------+--------+" << std::endl;
    }
    else {
        os << "ERROR: " << m.m_error_msg << std::endl;
    }
    return os;
}
