

#ifndef _Cluster_h_
#define _Cluster_h_

#include <JANA/JObject.h>

/// JObjects are plain-old data containers for inputs, intermediate results, and outputs.
/// They have member functions for introspection and maintaining associations with other JObjects, but
/// all of the numerical code which goes into their creation should live in a JFactory instead.
/// You are allowed to include STL containers and pointers to non-POD datatypes inside your JObjects,
/// however, it is highly encouraged to keep them flat and include only primitive datatypes if possible.
/// Think of a JObject as being a row in a database table, with event number as an implicit foreign key.

struct Cluster : public JObject {
    double x_center;     // Pixel coordinates centered around 0,0
    double y_center;     // Pixel coordinates centered around 0,0
    double E_tot;     // Energy loss in GeV
    double t_begin;   // Time in us
    double t_end;     // Time in us

    Cluster(double x_center, double y_center, double E_tot, double t_begin, double t_end)
            : x_center(x_center), y_center(y_center), E_tot(E_tot), t_begin(t_begin), t_end(t_end) {};


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
        summary.add(x_center, NAME_OF(x_center), "%f", "Pixel coords <- [0,80)");
        summary.add(y_center, NAME_OF(y_center), "%f", "Pixel coords <- [0,24)");
        summary.add(E_tot, NAME_OF(E_tot), "%f", "Energy loss in GeV");
        summary.add(t_begin, NAME_OF(t_begin), "%f", "Earliest observed time in us");
        summary.add(t_end, NAME_OF(t_end), "%f", "Latest observed time in us");
    }
};


#endif // _Cluster_h_

