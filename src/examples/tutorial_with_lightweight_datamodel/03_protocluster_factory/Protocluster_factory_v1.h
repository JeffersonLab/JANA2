
#pragma once
#include <JANA/JFactoryT.h>
#include <CalorimeterCluster.h>
#include "Protocluster_algorithm.h"

class Protocluster_factory_v1 : public JFactoryT<CalorimeterCluster> {
    // GlueX uses JFactoryT for all of their factories. JFactoryT is a templated JFactory
    // that assumes that each factory outputs exactly one databundle. This assumption
    // is a holdover from JANA1. Modern JANA2 `JFactory`s support much more sophisticated
    // outputs, as will be shown in the `v3` example here and later on. Because JFactoryT
    // inherits from JFactory, it is possible to sneak additional Outputs onto a JFactoryT
    // as well. However, if your factory needs multiple outputs, we recommend using the new
    // `v3` style instead.

private:
    // Parameter and calibration values are cached locally as member variables
    double m_energy_threshold;

    // The algorithm can also be a member variable, and store whatever local state it needs.
    // Just remember that there are multiple instances of each factory class in memory at the
    // same time, and that they are isolated from each other. Each factory will only see SOME
    // of the events in the event stream.
    Protocluster_algorithm m_algorithm;

public:
    Protocluster_factory_v1();

    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;
    void Finish() override;

};


