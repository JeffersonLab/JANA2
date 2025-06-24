// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>

struct CalorimeterCluster : public JObject {

    JOBJECT_PUBLIC(CalorimeterCluster)

    double x_center;     // x location of cluster center
    double y_center;     // y location of cluster center
    double z_center;     // z location of cluster center
    double energy;       // Total energy deposited [GeV]
    uint64_t time_begin; // Timestamp of earliest hit [ticks since timeframe start]
    uint64_t time_end;   // Timestamp of latest hit [ticks since timeframe start]


    void Summarize(JObjectSummary& summary) const override {
        summary.add(x_center, NAME_OF(x_center), "%f", "x location of cluster center");
        summary.add(y_center, NAME_OF(y_center), "%f", "y location of cluster center");
        summary.add(z_center, NAME_OF(z_center), "%f", "z location of cluster center");
        summary.add(energy, NAME_OF(energy), "%f", "Total energy deposited [GeV]");
        summary.add(time_begin, NAME_OF(t_begin), "%f", "Earliest timestamp [ticks since timeframe start]");
        summary.add(time_end, NAME_OF(t_end), "%f", "Latest timestamp [ticks since timeframe start]");
    }

};


