
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JObject.h>
#include <cstdint>

/// JObjects are plain-old data containers for inputs, intermediate results, and outputs.
/// They have member functions for introspection and maintaining associations with other JObjects, but
/// all of the numerical code which goes into their creation should live in a JFactory instead.
/// You are allowed to include STL containers and pointers to non-POD datatypes inside your JObjects,
/// however, it is highly encouraged to keep them flat and include only primitive datatypes if possible.
/// Think of a JObject as being a row in a database table, with event number as an implicit foreign key.

struct ADCHit : public JObject {

    JOBJECT_PUBLIC(F250Hit)

    uint32_t crate;
    uint32_t slot;
    uint32_t channel;
    uint32_t E;
    uint32_t t;


    /// Override Summarize to tell JANA how to produce a convenient string representation for our JObject.
    /// This can be used called from user code, but also lets JANA automatically inspect its own data. For instance,
    /// adding JCsvWriter<Hit> will automatically generate a CSV file containing each hit. Warning: This is obviously
    /// slow, so use this for debugging and monitoring but not inside the performance critical code paths.

    void Summarize(JObjectSummary& summary) const override {
        summary.add(crate, NAME_OF(crate), "%f");
        summary.add(slot, NAME_OF(slot), "%f");
        summary.add(channel, NAME_OF(channel), "%f");
        summary.add(E, NAME_OF(E), "%f", "Energy in GeV");
        summary.add(t, NAME_OF(t), "%f", "Time in ms");
    }
};


