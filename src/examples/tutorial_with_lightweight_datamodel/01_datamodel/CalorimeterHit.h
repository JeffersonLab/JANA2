
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>


struct CalorimeterHit : public JObject {

    JOBJECT_PUBLIC(CalorimeterHit)

    int x;     // Pixel coordinates centered around 0,0
    int y;     // Pixel coordinates centered around 0,0
    double E;  // Energy loss in GeV
    double t;  // Time in ms


    void Summarize(JObjectSummary& summary) const override {
        summary.add(x, NAME_OF(x), "%d", "Pixel coordinates centered around 0,0");
        summary.add(y, NAME_OF(y), "%d", "Pixel coordinates centered around 0,0");
        summary.add(E, NAME_OF(E), "%f", "Energy loss in GeV");
        summary.add(t, NAME_OF(t), "%f", "Time in ms");
    }
};


