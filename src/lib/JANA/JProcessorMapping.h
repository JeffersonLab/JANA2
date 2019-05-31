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

#ifndef JANA2_JWORKERMAPPING_H
#define JANA2_JWORKERMAPPING_H

#include <cstddef>
#include <ostream>
#include <vector>

class JProcessorMapping {

public:

    enum class AffinityStrategy { None, MemoryBound, ComputeBound, Error };
    enum class LocalityStrategy { Global, SocketLocal, NumaDomainLocal, CoreLocal, CpuLocal, Error };

    void initialize(AffinityStrategy affinity, LocalityStrategy locality);

    inline size_t get_loc_count() {
        return m_loc_count;
    }

    inline size_t get_cpu_id(size_t worker_id) {
        return (m_initialized) ? m_mapping[worker_id % m_mapping.size()].cpu_id : worker_id;
    }

    inline size_t get_loc_id(size_t worker_id) {
        return (m_initialized) ? m_mapping[worker_id % m_mapping.size()].location_id : 0;
    }

    friend std::ostream& operator<<(std::ostream& os, const JProcessorMapping& m);

private:

    struct Row {
        size_t location_id;
        size_t cpu_id;
        size_t core_id;
        size_t numa_domain_id;
        size_t socket_id;
        bool available;
    };

    AffinityStrategy m_affinity_strategy;
    LocalityStrategy m_locality_strategy;
    std::vector<Row> m_mapping;
    size_t m_loc_count;
    bool m_initialized;
    std::string m_error_msg;
};


#endif //JANA2_JWORKERMAPPING_H
