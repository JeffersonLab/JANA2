
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JTESTEVENTCONTEXTS_H
#define JANA2_JTESTEVENTCONTEXTS_H

#include <JANA/JObject.h>
#include <memory>

struct JTestEntangledEventData : public JObject {
    std::shared_ptr<std::vector<char>> buffer;

    JOBJECT_PUBLIC(JTestEntangledEventData)
};

struct JTestEventData : public JObject {
    std::vector<char> buffer;

    JOBJECT_PUBLIC(JTestEventData)

    void Summarize(JObjectSummary& summary) const override {
        summary.add(buffer.size(), "buffer_size", "%d");
    }
};

struct JTestTrackData : public JObject {
    std::vector<char> buffer;

    JOBJECT_PUBLIC(JTestTrackData)

    void Summarize(JObjectSummary& summary) const override {
        size_t nitems = std::min(buffer.size(), (size_t) 5);
        for (size_t i=0; i<nitems; ++i) {
            char varname[20];
            snprintf(varname, 20, "x_%ld", i);
            summary.add(buffer[i], varname, "%d");
        }
    }
};

struct JTestHistogramData : public JObject {
    std::vector<char> buffer;
    JOBJECT_PUBLIC(JTestHistogramData)
};

#endif //JANA2_JTESTEVENTCONTEXTS_H
