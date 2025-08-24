
#pragma once
#include <JANA/Components/JOmniFactory.h>
#include <CalorimeterHit.h>
#include <CalorimeterCluster.h>

// A `Config` struct is used to hold configuration parameters
struct ProtoclusterConfigs { double log_weight_energy = 5.0; };

// ePIC uses `JOmniFactory` as the base class for all of their factories.
// Note that while there is an implementation of `JOmniFactory` inside JANA2, it is slightly different
// from what ePIC uses. The main difference is that `eicrecon::JOmniFactory` has added support for
// an `spdlog` logger.

class Protocluster_factory_v2 : public JOmniFactory<Protocluster_factory_v2, ProtoclusterConfigs> {

private:

    // Inputs and outputs are now declared as members like so:
    Input<CalorimeterHit> m_hits_in {this};
    Output<CalorimeterCluster> m_clusters_out {this};

    // Note that factories support any number of inputs and outputs. Each input and output declaration may 
    // correspond to a single databundle, like above, or multiple, as will be shown in later examples.

    // Parameter values (and their defaults) are cached on the Config struct
    // Parameters themselves are declared as members like so:
    ParameterRef<double> m_log_weight_energy {this, "m_log_weight_energy", config().log_weight_energy };

    // Calibrations are not needed in this example, but they would be cached as member variables NOT on the Config struct.

    // The algorithm can also be a member variable, and store whatever local state it needs.
    // Just remember that there are multiple instances of each factory class in memory at the
    // same time, and that they are isolated from each other. Each factory will only see SOME
    // of the events in the event stream. If your algorithm is just a free function, that's preferred.

public:

    Protocluster_factory_v2();

    // JOmniFactory uses the Curiously Recurring Template Pattern to avoid introducing a second level of
    // virtual functions. This results in the callbacks we now use having different names, and also not
    // being declared as `virtual`. Note that `JEvent` is NOT exposed to user code anywhere.

    void Configure();
    void ChangeRun(int32_t run_number);
    void Execute(int32_t run_number, uint64_t event_number);

};


