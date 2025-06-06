
// Copyright 2020-2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>


struct CalorimeterHit : public JObject {

    JOBJECT_PUBLIC(CalorimeterHit)

    int cell_id; // Cell's id

    int row;  // Cell's row in detector plane (zero-indexed)
    int col;  // Cell's column in detector plane (zero-indexed)

    double x; // x location of cell's center [cm]
    double y; // y location of cell's center [cm]
    double z; // z location of cell's center [cm]

    double energy;  // Measured energy deposit [GeV]
    uint64_t time;  // Time [ns]


    void Summarize(JObjectSummary& summary) const override {
        summary.add(cell_id, NAME_OF(cell_id), "%d", "Cell ID");
        summary.add(row, NAME_OF(cell_id), "%d", "Cell row (zero-indexed)");
        summary.add(col, NAME_OF(cell_id), "%d", "Cell col (zero-indexed)");
        summary.add(x, NAME_OF(x), "%f", "x location of cell center [cm]");
        summary.add(y, NAME_OF(y), "%f", "y location of cell center [cm]");
        summary.add(z, NAME_OF(z), "%f", "z location of cell center [cm]");
        summary.add(energy, NAME_OF(energy), "%f", "Measured energy deposit [GeV]");
        summary.add(time, NAME_OF(time), "%d", "Time [ns]");
    }
};


