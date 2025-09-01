
// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JEventProcessor.h>
#include <CalorimeterHit.h>

class AsciiHeatmap_writer : public JEventProcessor {

    // Declare inputs
    Input<CalorimeterHit> m_hits_in {this};

    // Declare parameters
    // In a more realistic example, these would be obtained from the geometry interface
    Parameter<size_t> m_cell_cols {this, "cell_cols", 20, "Number of columns in the detector"};
    Parameter<size_t> m_cell_rows {this, "cell_rows", 10, "Number of rows in the detector"};

    // Declare the resource (or a handle to the resource) that is protected by this JEventProcessor
    std::unique_ptr<double[]> m_heatmap;

public:

    AsciiHeatmap_writer();
    virtual ~AsciiHeatmap_writer() = default;

    void Init() override;
    void ProcessSequential(const JEvent& event) override;
    void Finish() override;

};

