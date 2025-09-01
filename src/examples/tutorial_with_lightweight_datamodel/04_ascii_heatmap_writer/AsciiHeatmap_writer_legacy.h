
// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JEventProcessor.h>

class AsciiHeatmap_writer_legacy : public JEventProcessor {

    // Note that in the legacy style, we don't declare inputs.

    Parameter<size_t> m_cell_cols {this, "cell_cols", 20, "Number of columns in the detector"};
    Parameter<size_t> m_cell_rows {this, "cell_rows", 10, "Number of rows in the detector"};

    std::unique_ptr<double[]> m_heatmap;

public:

    AsciiHeatmap_writer_legacy();
    virtual ~AsciiHeatmap_writer_legacy() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};

