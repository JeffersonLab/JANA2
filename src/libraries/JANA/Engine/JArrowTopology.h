
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JARROWTOPOLOGY_H
#define JANA2_JARROWTOPOLOGY_H


#include <JANA/Services/JComponentManager.h>
#include <JANA/Status/JPerfMetrics.h>
#include <JANA/Utils/JEventPool.h>
#include <JANA/Utils/JProcessorMapping.h>

#include "JArrow.h"
#include "JMailbox.h"


struct JArrowTopology {
    enum class Status { Uninitialized, Paused, Running, Pausing, Draining, Finished };

    using Event = std::shared_ptr<JEvent>;
    using EventQueue = JMailbox<Event>;

    explicit JArrowTopology();
    virtual ~JArrowTopology();

    std::shared_ptr<JComponentManager> component_manager;
    // Ensure that ComponentManager stays alive at least as long as JArrowTopology does
    // Otherwise there is a potential use-after-free when JArrowTopology or JArrowProcessingController access components

    std::shared_ptr<JEventPool> event_pool; // TODO: Belongs somewhere else
    JPerfMetrics metrics;

    std::vector<JArrow*> arrows;
    std::vector<JArrow*> sources;           // Sources needed for activation
    std::vector<JArrow*> sinks;             // Sinks needed for finished message count // TODO: Not anymore
    std::vector<EventQueue*> queues;        // Queues shared between arrows
    JProcessorMapping mapping;

    std::atomic<Status> m_current_status {Status::Uninitialized};
    std::atomic_int64_t running_arrow_count {0};  // Detects when the topology has paused
    // int64_t running_worker_count = 0;          // Detects when the workers have all joined

    size_t event_pool_size = 1;                   //  Will be defaulted to nthreads by builder
    bool limit_total_events_in_flight = true;
    bool enable_call_graph_recording = false;
    size_t event_queue_threshold = 80;
    size_t event_source_chunksize = 40;
    size_t event_processor_chunksize = 1;
    size_t location_count = 1;
    bool enable_stealing = false;
    int affinity = 0; // By default, don't pin the CPU at all
    int locality = 0; // By default, assume no NUMA domains

    JLogger m_logger;

    void initialize();
    void drain();
    void run(int nthreads);
    void request_pause();
    void achieve_pause();
    void finish();

};


#endif //JANA2_JARROWTOPOLOGY_H
