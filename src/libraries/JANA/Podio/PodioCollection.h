
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_PODIOCOLLECTION_H
#define JANA2_PODIOCOLLECTION_H

#include <vector>
#include <JANA/JObject.h>

struct PodioCollection {
    std::vector<JObject*> data;

    inline ~PodioCollection() {
        for (auto* datum : data) {
            delete datum;
        }
    }
};

#endif //JANA2_PODIOCOLLECTION_H
