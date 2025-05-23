
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_RAWHIT_H
#define JANA2_RAWHIT_H

#include <JANA/JObject.h>
#include <JANA/JException.h>

struct AHit : public JObject {
    double E, t, x, y, z;
};

#endif //JANA2_RAWHIT_H
