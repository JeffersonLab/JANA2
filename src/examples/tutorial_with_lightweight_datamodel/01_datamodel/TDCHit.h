
// Copyright 2020-2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>


struct TDCHit: public JObject {

    JOBJECT_PUBLIC(TDCHit)

    uint32_t cell_id; // Cell's id

    double x; // x location of cell's center [cm]
    double y; // y location of cell's center [cm]
    double z; // z location of cell's center [cm]

    double time;  // Time [ns]


    // Define a constructor that forces all fields to be initialized
    TDCHit(uint32_t cell_id, double x, double y, double z, uint64_t time)
    : cell_id(cell_id), x(x), y(y), z(z), time(time) {}


    void Summarize(JObjectSummary& summary) const override {
        summary.add(cell_id, NAME_OF(cell_id), "%d", "Cell ID");
        summary.add(x, NAME_OF(x), "%f", "x location of cell center [cm]");
        summary.add(y, NAME_OF(y), "%f", "y location of cell center [cm]");
        summary.add(z, NAME_OF(z), "%f", "z location of cell center [cm]");
        summary.add(time, NAME_OF(time), "%d", "Time [ns]");
    }
};


