

#ifndef _Hit_h_
#define _Hit_h_

#include <JANA/JObject.h>

/// JObjects are plain-old data containers for inputs, intermediate results, and outputs.
/// They have member functions for introspection and maintaining associations with other JObjects, but
/// all of the numerical code which goes into their creation should live in a JFactory instead.
/// You are allowed to include STL containers and pointers to non-POD datatypes inside your JObjects,
/// however, it is highly encouraged to keep them flat and include only primitive datatypes if possible.
/// Think of a JObject as being a row in a database table, with event number as an implicit foreign key.

struct Hit : public JObject {
    int x;     // Pixel coordinates centered around 0,0
    int y;     // Pixel coordinates centered around 0,0
    double E;  // Energy loss in GeV
    double t;  // Time in ms


    /// Make it convenient to construct one of these things
    Hit(int x, int y, double E, double t) : x(x), y(y), E(E), t(t) {};


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
        summary.add(x, NAME_OF(x), "%d", "Pixel coordinates centered around 0,0");
        summary.add(y, NAME_OF(y), "%d", "Pixel coordinates centered around 0,0");
        summary.add(E, NAME_OF(E), "%f", "Energy loss in GeV");
        summary.add(t, NAME_OF(t), "%f", "Time in ms");
    }
};


#endif // _Hit_h_

