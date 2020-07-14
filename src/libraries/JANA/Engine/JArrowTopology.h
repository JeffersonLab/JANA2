
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JARROWTOPOLOGY_H
#define JANA2_JARROWTOPOLOGY_H


#include <JANA/Services/JComponentManager.h>
#include <JANA/Status/JPerfMetrics.h>
#include <JANA/Utils/JProcessorMapping.h>
#include <JANA/Utils/JEventPool.h>

#include "JActivable.h"
#include "JArrow.h"
#include "JMailbox.h"


struct JArrowTopology : public JActivable {

    using Event = std::shared_ptr<JEvent>;
    using EventQueue = JMailbox<Event>;

    enum Status { Inactive, Running, Draining, Finished };

    explicit JArrowTopology();
    virtual ~JArrowTopology();

    static JArrowTopology* from_components(std::shared_ptr<JComponentManager>, std::shared_ptr<JParameterManager>, int nthreads);

    std::shared_ptr<JComponentManager> component_manager;
    // Ensure that ComponentManager stays alive at least as long as JArrowTopology does
    // Otherwise there is a potential use-after-free when JArrowTopology or JArrowProcessingController access components

    std::shared_ptr<JEventPool> event_pool; // TODO: Belongs somewhere else
    JPerfMetrics metrics;
    Status status = Inactive; // TODO: Merge this concept with JActivable

    std::vector<JArrow*> arrows;
    std::vector<JArrow*> sources;           // Sources needed for activation
    std::vector<JArrow*> sinks;             // Sinks needed for finished message count // TODO: Not anymore
    std::vector<EventQueue*> queues;        // Queues shared between arrows
    JProcessorMapping mapping;

    JLogger _logger;

    bool is_active() override;
    void set_active(bool is_active) override;
};


#endif //JANA2_JARROWTOPOLOGY_H
