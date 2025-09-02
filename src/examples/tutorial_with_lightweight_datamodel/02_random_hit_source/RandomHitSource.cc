
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "RandomHitSource.h"

#include "CalorimeterHit.h"
#include <random>
#include <algorithm>


RandomHitSource::RandomHitSource() : JEventSource() {
    SetTypeName(NAME_OF_THIS); // Used for error messages
    SetPrefix("random_hit_source");       // Used for log level and parameters
    SetCallbackStyle(CallbackStyle::ExpertMode); // Use new-style callbacks
}

void RandomHitSource::Init() {
    if (*m_expected_clusters_per_event < 1) {
        throw JException("expected_clusters_per_event must be >= 1. Actual value=%d", *m_expected_clusters_per_event);
    }

    LOG_INFO(GetLogger()) << "RandomHitSource is using seed=" << *m_seed;
    m_rng.seed(*m_seed);
}

JEventSource::Result RandomHitSource::Emit(JEvent& event) {

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.
    /// In this case, the shared state that we update is the random number generator m_rng

    std::uniform_int_distribution<> hit_row_distribution(0, *m_cell_rows-2);
    std::uniform_int_distribution<> hit_col_distribution(0, *m_cell_cols-2);
    std::uniform_int_distribution<> intercluster_time_distribution(1, std::max<int>(2, 2 * (*m_event_width_ns) / (*m_expected_clusters_per_event)));
    std::normal_distribution<> energy_distribution(100, 10);

    uint64_t event_finish_time_ns = m_event_start_timestamp_ns + *m_event_width_ns;
    uint64_t current_time_ns = m_event_start_timestamp_ns + intercluster_time_distribution(m_rng);

    std::vector<CalorimeterHit*> output_hits;

    while (current_time_ns < event_finish_time_ns) {
        auto cluster_row = hit_row_distribution(m_rng);
        auto cluster_col = hit_col_distribution(m_rng);

        // For simplicity, let's have each particle produce hits within a 2x2 grid of cells
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 2; ++col) {

                auto hit = new CalorimeterHit(0, 0, 0, 0, 0, 0, 0, 0);
                hit->row = cluster_row + row;
                hit->col = cluster_col + col;
                hit->x = hit->col * 10; // Pretend each cell is 10cm x 10cm
                hit->y = hit->row * 10;
                hit->z = 500; // Pretend detector plane is 500cm behind vertex
                hit->time = current_time_ns;
                hit->energy = energy_distribution(m_rng);
                output_hits.push_back(hit);
            }
        }
        current_time_ns += intercluster_time_distribution(m_rng);
    }

    event.Insert(output_hits, "rechits");
    m_event_start_timestamp_ns += *m_event_width_ns;
    return Result::Success;
}

std::string RandomHitSource::GetDescription() {

    /// GetDescription() helps JANA explain to the user what is going on
    return "Generates random hits on flat calorimeter";
}


