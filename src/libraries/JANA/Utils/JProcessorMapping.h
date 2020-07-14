
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JWORKERMAPPING_H
#define JANA2_JWORKERMAPPING_H

#include <cstddef>
#include <ostream>
#include <vector>

class JProcessorMapping {

public:

    enum class AffinityStrategy { None, MemoryBound, ComputeBound };
    enum class LocalityStrategy { Global, SocketLocal, NumaDomainLocal, CoreLocal, CpuLocal };

    void initialize(AffinityStrategy affinity, LocalityStrategy locality);

    inline size_t get_loc_count() const {
        return m_loc_count;
    }

    inline size_t get_cpu_id(size_t worker_id) const {
        return (m_initialized) ? m_mapping[worker_id % m_mapping.size()].cpu_id : worker_id;
    }

    inline size_t get_loc_id(size_t worker_id) const {
        return (m_initialized) ? m_mapping[worker_id % m_mapping.size()].location_id : 0;
    }

    inline AffinityStrategy get_affinity() const {
        return m_affinity_strategy;
    }

    inline LocalityStrategy get_locality() const {
        return m_locality_strategy;
    }

    friend std::ostream& operator<<(std::ostream& os, const JProcessorMapping& m);
    friend std::ostream& operator<<(std::ostream& os, const AffinityStrategy& s);
    friend std::ostream& operator<<(std::ostream& os, const LocalityStrategy& s);

private:

    struct Row {
        size_t location_id;
        size_t cpu_id;
        size_t core_id;
        size_t numa_domain_id;
        size_t socket_id;
    };

    AffinityStrategy m_affinity_strategy = AffinityStrategy::None;
    LocalityStrategy m_locality_strategy = LocalityStrategy::Global;
    std::vector<Row> m_mapping;
    size_t m_loc_count = 1;
    bool m_initialized = false;
    std::string m_error_msg = "Not initialized yet";
};


#endif //JANA2_JWORKERMAPPING_H
