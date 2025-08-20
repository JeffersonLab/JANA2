
#pragma once
#include <JANA/Components/JOmniFactory.h>
#include <CalorimeterHit.h>
#include <CalorimeterCluster.h>
#include "Protocluster_algorithm.h"

// If you are starting a new project, we recommend using the JFactory base class directly, since
// it now has almost all of the functionality of JFactoryT and JOmniFactory, but with a simpler interface.

class Protocluster_factory_v3 : public JFactory {

private:

    // Declare inputs

    Input<CalorimeterHit> m_hits_in {this};

    // Declare outputs

    Output<CalorimeterCluster> m_clusters_out {this};

    // Declare parameters and services

    Parameter<double> m_energy_threshold {this, "energy_threshold", 5.0 };

    // The algorithm can also be a member variable, and store whatever local state it needs.
    // Just remember that there are multiple instances of each factory class in memory at the
    // same time, and that they are isolated from each other. Each factory will only see SOME
    // of the events in the event stream.

    Protocluster_algorithm m_algorithm;

public:

    Protocluster_factory_v3();

    void Init() override;
    void ChangeRun(const JEvent& event) override;
    // TODO: void Process(const JEvent& event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;
    void Finish() override;

};


