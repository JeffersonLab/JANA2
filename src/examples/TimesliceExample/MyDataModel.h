// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JObject.h>

struct MyHit : public JObject {
    int hit_id;
    int energy, x, y;
};

struct MyCluster : public JObject {
    int cluster_id;
    int energy, x, y;
    std::vector<int> hits;
};

