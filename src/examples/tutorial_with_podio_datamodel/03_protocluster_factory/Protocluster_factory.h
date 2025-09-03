
#pragma once
#include <JANA/Components/JOmniFactory.h>
#include <jana2_tutorial_podio_datamodel/CalorimeterHitCollection.h>
#include <jana2_tutorial_podio_datamodel/CalorimeterClusterCollection.h>

// If you are starting a new project, we recommend using the JFactory base class directly, since
// it now has almost all of the functionality of JFactoryT and JOmniFactory, but with a simpler interface.

class Protocluster_factory : public JFactory {

private:

    // Declare inputs

    PodioInput<CalorimeterHit> m_hits_in {this};

    // Declare outputs

    PodioOutput<CalorimeterCluster> m_clusters_out {this, "proto"};

    // Declare parameters and services

    Parameter<double> m_log_weight_energy {this, "log_weight_energy", 5.0 };

    // The algorithm can also be a member variable, and store whatever local state it needs.
    // Just remember that there are multiple instances of each factory class in memory at the
    // same time, and that they are isolated from each other. Each factory will only see SOME
    // of the events in the event stream. If your algorithm is just a free function, that's preferred.

public:

    Protocluster_factory();

    void Init() override;
    void ChangeRun(const JEvent& event) override;
    void Process(const JEvent& event) override;
    void Finish() override;

};


