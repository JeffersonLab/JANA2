
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

struct ADCPulse: public JObject {

    JOBJECT_PUBLIC(ADCPulse)

    uint32_t crate;
    uint32_t slot;
    uint32_t channel;

    uint32_t amplitude;
    uint32_t pedestal;
    uint32_t integral;
    uint32_t timestamp;

    void Summarize(JObjectSummary& summary) const override {
        summary.add(crate, NAME_OF(crate), "%d");
        summary.add(slot, NAME_OF(slot), "%d");
        summary.add(channel, NAME_OF(channel), "%d");
        summary.add(amplitude, NAME_OF(amplitude), "%d", "Amplitude");
        summary.add(pedestal, NAME_OF(pedestal), "%d", "Pedestal");
        summary.add(integral, NAME_OF(integral), "%d", "Integral");
        summary.add(timestamp, NAME_OF(timestamp), "%d", "Timestamp");
    }
};


