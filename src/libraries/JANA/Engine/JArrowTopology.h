
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JARROWTOPOLOGY_H
#define JANA2_JARROWTOPOLOGY_H


#include <JANA/Services/JComponentManager.h>
#include <JANA/Status/JPerfMetrics.h>
#include <JANA/Utils/JEventPool.h>
#include <JANA/Utils/JProcessorMapping.h>

#include "JActivable.h"
#include "JArrow.h"
#include "JMailbox.h"


struct JArrowTopology : public JActivable {

    using Event = std::shared_ptr<JEvent>;
    using EventQueue = JMailbox<Event>;

    enum Status { Inactive, Running, Draining, Finished };

    explicit JArrowTopology();
    virtual ~JArrowTopology();

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

    size_t event_pool_size;                 //  Will be defaulted to nthreads later
    bool limit_total_events_in_flight = true;
    bool enable_call_graph_recording = false;
    size_t event_queue_threshold = 80;
    size_t event_source_chunksize = 40;
    size_t event_processor_chunksize = 1;
    size_t location_count = 1;
    bool enable_stealing = false;
    int affinity = 2;
    int locality = 0;

    JLogger m_logger;

    bool is_active() override;
    void set_active(bool is_active) override;
};


#endif //JANA2_JARROWTOPOLOGY_H
