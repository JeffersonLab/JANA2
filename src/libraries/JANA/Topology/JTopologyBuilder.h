
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrowTopology.h>

#include <memory>

class JFoldArrow;
class JUnfoldArrow;
class JServiceLocator;

class JTopologyBuilder : public JService {

    std::shared_ptr<JParameterManager> m_params;
    std::shared_ptr<JComponentManager> m_components;
    std::shared_ptr<JArrowTopology> m_topology;
    std::function<std::shared_ptr<JArrowTopology>(std::shared_ptr<JArrowTopology>)> m_configure_topology;

    size_t m_event_pool_size = 4;
    size_t m_event_queue_threshold = 80;
    size_t m_event_source_chunksize = 40;
    size_t m_event_processor_chunksize = 1;
    size_t m_location_count = 1;
    bool m_enable_call_graph_recording = false;
    bool m_enable_stealing = false;
    bool m_limit_total_events_in_flight = true;
    int m_affinity = 0;
    int m_locality = 0;
    JLogger m_arrow_logger;
    JLogger m_queue_logger;
    JLogger m_builder_logger;

public:

    JTopologyBuilder() = default;

    ~JTopologyBuilder() override = default;

    std::string print_topology();

    /// set allows the user to specify a topology directly. Note that this needs to be set before JApplication::Initialize
    /// gets called, which means that you won't be able to include components loaded from plugins. You probably want to use
    /// JTopologyBuilder::set_configure_fn instead, which does give you that access.
    void set(std::shared_ptr<JArrowTopology> topology);

    /// set_cofigure_fn lets the user provide a lambda that sets up a topology after all components have been loaded.
    /// It provides an 'empty' JArrowTopology which has been furnished with a pointer to the JComponentManager, the JEventPool,
    /// and the JProcessorMapping (in case you care about NUMA details). However, it does not contain any queues or arrows.
    /// You have to furnish those yourself.
    void set_configure_fn(std::function<std::shared_ptr<JArrowTopology>(std::shared_ptr<JArrowTopology>)> configure_fn);

    std::shared_ptr<JArrowTopology> get_or_create();

    void acquire_services(JServiceLocator *sl) override;

    std::shared_ptr<JArrowTopology> create_empty();

    void attach_lower_level(JEventLevel current_level, JUnfoldArrow* parent_unfolder, JFoldArrow* parent_folder, bool found_sink);

    void attach_top_level(JEventLevel current_level);

};


