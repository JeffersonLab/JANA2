
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

struct EventHeader : public JObject {

    JOBJECT_PUBLIC(EventHeader)

    int64_t run_number;
    int64_t timeframe_number;
    int64_t event_number;

    void Summarize(JObjectSummary& summary) const override {
        summary.add(run_number, NAME_OF(run_number), "%d", "Run number");
        summary.add(timeframe_number, NAME_OF(timeframe_number), "%d", "Timeframe number");
        summary.add(event_number, NAME_OF(event_number), "%d", "Event number");
    }
};


