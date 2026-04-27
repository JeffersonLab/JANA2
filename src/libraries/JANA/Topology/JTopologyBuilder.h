
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <memory>

#include <JANA/JService.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Topology/JEventQueue.h>
#include <JANA/Topology/JEventPool.h>
#include <JANA/Utils/JEventLevel.h>
#include <JANA/Utils/JProcessorMapping.h>


class JParameterManager;
class JComponentManager;
class JArrow;
class JFoldArrow;
class JUnfoldArrow;
class JEventTapArrow;

class JTopologyBuilder : public JService {
    // Services
    Service<JParameterManager> m_params {this};
    std::shared_ptr<JComponentManager> m_components;

    // The topology itself
    std::vector<JArrow*> arrows;
    std::vector<JEventQueue*> queues;            // Queues shared between arrows
    std::vector<JEventPool*> pools;          // Pools shared between arrows

    std::map<std::string, JArrow*> arrow_lookup;
    std::map<JEventLevel, JEventPool*> pool_lookup;

    // Topology configuration
    std::map<JEventLevel, size_t> m_max_inflight_events;
    size_t m_location_count = 1;
    //bool m_enable_stealing = false;
    int m_affinity = 0;
    int m_locality = 0;

    std::function<void(JTopologyBuilder&, JComponentManager&)> m_configure_topology;
    JProcessorMapping mapping;

public:

    JTopologyBuilder();
    ~JTopologyBuilder() override;

    void Init() override;

    void AddArrow(JArrow* arrow);

    void ConnectQueue(std::string upstream_arrow_name, std::string upstream_port_name,
                      std::string downstream_arrow_name, std::string downstream_port_name);

    void ConnectPool(std::string arrow_name, std::string port_name, JEventLevel level);

    void ConnectPool(JEventLevel upstream_level, JEventLevel downstream_level);


    /// SetConfigureFn() lets the user manually set up a topology after all components have been loaded.
    /// This is meant to be used with AddArrow(), ConnectPool(), and ConnectQueue().
    void SetConfigureFn(std::function<void(JTopologyBuilder&, JComponentManager&)> configure_fn);

    void CreateTopology();

    std::string PrintTopology();

    const std::vector<JArrow*>& GetArrows() { return arrows; };
    const std::vector<JEventPool*>& GetPools() { return pools; };
    const std::vector<JEventQueue*>& GetQueues() { return queues; };
    const JProcessorMapping& GetProcessorMapping() { return mapping; };

private:
    void attach_level(JEventLevel current_level, JUnfoldArrow* parent_unfolder, JFoldArrow* parent_folder);
    void connect_to_first_available(JArrow* upstream, size_t upstream_port_id, std::vector<std::pair<JArrow*, size_t>> downstreams);
    void connect(JArrow* upstream, size_t upstream_port_id, JArrow* downstream, size_t downstream_port_id);
    std::pair<JEventTapArrow*, JEventTapArrow*> create_tap_chain(std::vector<JEventProcessor*>& procs, std::string name);
    JEventPool* GetOrCreatePool(JEventLevel level);
};


