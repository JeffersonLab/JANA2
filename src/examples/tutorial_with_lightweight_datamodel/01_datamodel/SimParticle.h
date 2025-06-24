
// Copyright 2020-2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

struct SimParticle : public JObject {

    JOBJECT_PUBLIC(SimParticle)

    double momentum_z;       // Momentum in the z direction
    double momentum_theta;   // Momentum in the theta direction
    double momentum_phi;     // Momentum in the phi direction
    double energy;           // Total energy
    uint32_t pdg;            // PDG particle id


    void Summarize(JObjectSummary& summary) const override {
        summary.add(momentum_z, NAME_OF(momentum_z), "%f");
        summary.add(momentum_theta, NAME_OF(momentum_theta), "%f");
        summary.add(momentum_phi, NAME_OF(momentum_phi), "%f");
        summary.add(energy, NAME_OF(energy), "%f");
        summary.add(pdg, NAME_OF(pdg), "%d");
    }
};


