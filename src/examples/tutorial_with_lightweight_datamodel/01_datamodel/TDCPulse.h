
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

struct TDCPulse: public JObject {

    JOBJECT_PUBLIC(TDCPulse)

    uint32_t crate;
    uint32_t slot;
    uint32_t channel;

    uint32_t coarse_time;
    uint32_t fine_time;
    bool is_leading;

    void Summarize(JObjectSummary& summary) const override {
        summary.add(crate, NAME_OF(crate), "%d");
        summary.add(slot, NAME_OF(slot), "%d");
        summary.add(channel, NAME_OF(channel), "%d");
        summary.add(coarse_time, NAME_OF(coarse_time), "%d");
        summary.add(fine_time, NAME_OF(fine_time), "%d");
        summary.add(is_leading, NAME_OF(is_leading), "%d");
    }
};


