
// Copyright 2020-2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "RandomHitSource.h"

#include <jana2_tutorial_podio_datamodel/CalorimeterHitCollection.h>
#include <random>


RandomHitSource::RandomHitSource() : JEventSource() {
    SetTypeName(NAME_OF_THIS);
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

void RandomHitSource::Init() {
    m_rng.seed(*m_seed);
}

JEventSource::Result RandomHitSource::Emit(JEvent& event) {

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.
    /// In this case, the shared state that we update is the random number generator m_rng

    std::uniform_int_distribution<> hit_row_distribution(0, *m_cell_rows-1);
    std::uniform_int_distribution<> hit_col_distribution(0, *m_cell_cols-1);
    std::uniform_int_distribution<> intercluster_time_distribution(0, 2 * (*m_event_width_ns) / (*m_expected_clusters_per_event));
    std::normal_distribution<> energy_distribution(100, 10);

    uint64_t event_finish_time_ns = m_event_start_timestamp_ns + *m_event_width_ns;
    uint64_t current_time_ns = m_event_start_timestamp_ns + intercluster_time_distribution(m_rng);

    CalorimeterHitCollection output_hits;

    while (current_time_ns < event_finish_time_ns) {
        auto cluster_row = hit_row_distribution(m_rng);
        auto cluster_col = hit_col_distribution(m_rng);

        // For simplicity, let's have each particle produce hits within a 2x2 grid of cells
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 2; ++col) {

                MutableCalorimeterHit hit;

                hit.setRow(cluster_row + row);
                hit.setCol(cluster_col + col);
                hit.setX(hit.getCol() * 10); // Pretend each cell is 10cm x 10cm
                hit.setY(hit.getRow() * 10);
                hit.setZ(500); // Pretend detector plane is 500cm behind vertex
                hit.setTime(current_time_ns);
                hit.setEnergy(energy_distribution(m_rng));

                output_hits.push_back(std::move(hit));
            }
        }
        current_time_ns += intercluster_time_distribution(m_rng);
    }

    event.InsertCollection<CalorimeterHit>(std::move(output_hits), "CalorimeterHit:raw");
    m_event_start_timestamp_ns += *m_event_width_ns;
    return Result::Success;
}

std::string RandomHitSource::GetDescription() {

    /// GetDescription() helps JANA explain to the user what is going on
    return "Generates random hits on flat calorimeter";
}


