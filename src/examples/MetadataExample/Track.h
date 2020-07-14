
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _Track_h_
#define _Track_h_

#include <JANA/JObject.h>


/// A super basic Track model.
struct Track : public JObject {
    int pid;
    double x;  // mm
    double y;  // mm
    double z;  // mm
    double px; // GeV
    double py; // GeV
    double pz; // GeV
    double t;  // ms

    /// Make it convenient to construct one of these things
    Track(int pid, double x, double y, double z, double px, double py, double pz, double t)
        : pid(pid), x(x), y(y), z(z), px(px), py(py), pz(pz), t(t) {
    }

    /// Override className to tell JANA to store the exact name of this class where we can
    /// access it at runtime. JANA provides a NAME_OF_THIS macro so that this will return the correct value
    /// even if you rename the class using automatic refactoring tools.

    const std::string className() const override {
        return NAME_OF_THIS;
    }

    /// Override Summarize to tell JANA how to produce a convenient string representation for our JObject.
    /// This can be used called from user code, but also lets JANA automatically inspect its own data. For instance,
    /// adding JCsvWriter<Hit> will automatically generate a CSV file containing each hit. Warning: This is obviously
    /// slow, so use this for debugging and monitoring but not inside the performance critical code paths.

    void Summarize(JObjectSummary& summary) const override {
        summary.add(pid, NAME_OF(pid), "%d", "Particle PID");
        summary.add(x, NAME_OF(x), "%d", "Pixel coordinates centered around 0,0 [mm]");
        summary.add(y, NAME_OF(y), "%d", "Pixel coordinates centered around 0,0 [mm]");
        summary.add(z, NAME_OF(z), "%d", "Pixel coordinates centered around 0,0 [mm]");
        summary.add(px, NAME_OF(px), "%d", "Momentum in x direction [GeV]");
        summary.add(py, NAME_OF(py), "%d", "Momentum in y direction [GeV]");
        summary.add(pz, NAME_OF(pz), "%d", "Momentum in z direction [GeV]");
        summary.add(t, NAME_OF(t), "%d", "Time in ms");
    }
};


#endif // _Track_h_

