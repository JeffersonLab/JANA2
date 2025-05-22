
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

struct SimParticle : public JObject {

    JOBJECT_PUBLIC(SimParticle)

    double mom_z;
    double mom_theta;
    double mom_phi;
    double energy;
    uint32_t pdg;


    void Summarize(JObjectSummary& summary) const override {
        summary.add(mom_z, NAME_OF(mom_r), "%f");
        summary.add(mom_theta, NAME_OF(mom_theta), "%f");
        summary.add(mom_phi, NAME_OF(mom_phi), "%f");
        summary.add(energy, NAME_OF(energy), "%f");
        summary.add(pdg, NAME_OF(pdg), "%d");
    }
};


