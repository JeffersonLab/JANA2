
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_PODIOFRAME_H
#define JANA2_PODIOFRAME_H


#include "PodioCollection.h"

struct PodioFrame {
    std::map<std::string, PodioCollection*> store;

    inline ~PodioFrame() {
        for (auto& pair : store) {
            delete pair.second;
        }
    }
};

#endif //JANA2_PODIOFRAME_H
