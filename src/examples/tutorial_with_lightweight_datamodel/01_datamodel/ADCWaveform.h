
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

struct ADCWaveform: public JObject {

    JOBJECT_PUBLIC(ADCWaveform)

    uint32_t crate;
    uint32_t slot;
    uint32_t channel;

    uint32_t pedestal;
    uint32_t timestamp;
    std::vector<uint32_t> samples;

    void Summarize(JObjectSummary& summary) const override {

        std::ostringstream oss;
        for (auto sample: samples) {
            oss << sample << ", ";
        }
        
        summary.add(crate, NAME_OF(crate), "%d");
        summary.add(slot, NAME_OF(slot), "%d");
        summary.add(channel, NAME_OF(channel), "%d");
        summary.add(pedestal, NAME_OF(pedestal), "%d");
        summary.add(timestamp, NAME_OF(timestamp), "%d");
        summary.add(oss.str().c_str(), NAME_OF(samples), "%s");
    }
};


