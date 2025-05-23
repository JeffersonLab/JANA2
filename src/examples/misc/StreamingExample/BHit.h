
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_RAWHIT_H
#define JANA2_RAWHIT_H

#include <JANA/JObject.h>
#include <JANA/JException.h>

struct BHit : public JObject {
    double E, t;
    int sector, layer;
};


#endif //JANA2_RAWHIT_H
