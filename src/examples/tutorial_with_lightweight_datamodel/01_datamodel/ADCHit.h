
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

struct ADCHit : public JObject {

    JOBJECT_PUBLIC(ADCHit)

    uint32_t crate;
    uint32_t slot;
    uint32_t channel;
    uint32_t energy;
    uint32_t timestamp;

    void Summarize(JObjectSummary& summary) const override {
        summary.add(crate, NAME_OF(crate), "%f");
        summary.add(slot, NAME_OF(slot), "%f");
        summary.add(channel, NAME_OF(channel), "%f");
        summary.add(energy, NAME_OF(energy), "%f", "Energy in GeV");
        summary.add(timestamp, NAME_OF(timestamp), "%f", "Time in ticks since timeframe start");
    }
};


