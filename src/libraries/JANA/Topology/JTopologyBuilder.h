
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <memory>
#include <JANA/JService.h>
#include <JANA/Utils/JProcessorMapping.h>
#include <JANA/Topology/JEventQueue.h>
#include <JANA/Topology/JEventPool.h>

#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JComponentManager.h>


class JParameterManager;
class JComponentManager;
class JArrow;
class JFoldArrow;
class JUnfoldArrow;
class JEventTapArrow;

class JTopologyBuilder : public JService {
public:
    // Services
    Service<JParameterManager> m_params {this};
    std::shared_ptr<JComponentManager> m_components;

    // The topology itself
    std::vector<JArrow*> arrows;
    std::vector<JEventQueue*> queues;            // Queues shared between arrows
    std::vector<JEventPool*> pools;          // Pools shared between arrows
    
    // Topology configuration
    size_t m_max_inflight_events = 1;
    size_t m_location_count = 1;
    bool m_enable_stealing = false;
    int m_affinity = 0;
    int m_locality = 0;

    // Things that probably shouldn't be here
    std::function<void(JTopologyBuilder&)> m_configure_topology;
    JProcessorMapping mapping;

public:

    JTopologyBuilder();
    ~JTopologyBuilder() override;

    void acquire_services(JServiceLocator *sl) override;

    /// set_cofigure_fn lets the user provide a lambda that sets up a topology after all components have been loaded.
    /// It provides an 'empty' JArrowTopology which has been furnished with a pointer to the JComponentManager, the JEventPool,
    /// and the JProcessorMapping (in case you care about NUMA details). However, it does not contain any queues or arrows.
    /// You have to furnish those yourself.
    void set_configure_fn(std::function<void(JTopologyBuilder&)> configure_fn);

    void create_topology();

    void attach_level(JEventLevel current_level, JUnfoldArrow* parent_unfolder, JFoldArrow* parent_folder);
    void connect_to_first_available(JArrow* upstream, size_t upstream_port_id, std::vector<std::pair<JArrow*, size_t>> downstreams);
    void connect(JArrow* upstream, size_t upstream_port_id, JArrow* downstream, size_t downstream_port_id);
    std::pair<JEventTapArrow*, JEventTapArrow*> create_tap_chain(std::vector<JEventProcessor*>& procs, std::string name);

    std::string print_topology();


};


