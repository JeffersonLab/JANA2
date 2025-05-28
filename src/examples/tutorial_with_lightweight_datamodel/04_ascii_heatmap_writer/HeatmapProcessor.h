
// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JEventProcessor.h>

class HeatmapProcessor : public JEventProcessor {

    Parameter<int> m_cell_cols {this, "cell_cols", 20, "Number of columns in the detector"};
    Parameter<int> m_cell_rows {this, "cell_rows", 10, "Number of rows in the detector"};

    std::unique_ptr<double[]> m_heatmap;

public:

    HeatmapProcessor();
    virtual ~HeatmapProcessor() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};

