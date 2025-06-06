
// Copyright 2020-2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JEventSource.h>
#include <random>

class RandomHitSource : public JEventSource {

    Parameter<int> m_seed {this, "seed", 0, "Random seed"};
    Parameter<uint32_t> m_event_width_ns {this, "event_width", 20, "Event width [ns]"};
    Parameter<uint32_t> m_expected_clusters_per_event {this, "expected_clusters_per_event", 10, "Expected clusters per event"};
    Parameter<int> m_cell_cols {this, "cell_cols", 20, "Number of columns in the detector"};
    Parameter<int> m_cell_rows {this, "cell_rows", 10, "Number of rows in the detector"};

    uint32_t m_event_start_timestamp_ns = 0;
    std::mt19937 m_rng;

public:

    RandomHitSource();
    virtual ~RandomHitSource() = default;

    void Init() override;
    Result Emit(JEvent&) override;

    static std::string GetDescription();
};

