
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JTOPOLOGYBUILDER_H
#define JANA2_JTOPOLOGYBUILDER_H

#include <JANA/Engine/JArrowTopology.h>
#include "JEventSourceArrow.h"
#include "JEventProcessorArrow.h"
#include <memory>

class JTopologyBuilder : public JService {

    std::shared_ptr<JParameterManager> m_params;
    std::shared_ptr<JComponentManager> m_components;
    std::shared_ptr<JArrowTopology> m_topology;

    size_t m_event_pool_size = 4;
    size_t m_event_queue_threshold = 80;
    size_t m_event_source_chunksize = 40;
    size_t m_event_processor_chunksize = 1;
    size_t m_location_count = 1;
    bool m_enable_call_graph_recording = false;
    bool m_enable_stealing = false;
    bool m_limit_total_events_in_flight = true;
    int m_affinity = 2;
    int m_locality = 0;

public:

    JTopologyBuilder() = default;

    ~JTopologyBuilder() override = default;

    inline void set(std::shared_ptr<JArrowTopology> topology) {
        m_topology = topology;
    }

    inline std::shared_ptr<JArrowTopology> get() {
        return m_topology;
    }

    inline std::shared_ptr<JArrowTopology> get_or_create() {
        if (m_topology == nullptr) {
            create_default_topology();
        }
        return m_topology;
    }


    void acquire_services(JServiceLocator *sl) override {
        m_components = sl->get<JComponentManager>();
        m_params = sl->get<JParameterManager>();

        // We default event pool size to be equal to nthreads
        // We parse the 'nthreads' parameter two different ways for backwards compatibility.
        if (m_params->Exists("nthreads")) {
            if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
                m_event_pool_size = JCpuInfo::GetNumCpus();
            } else {
                m_event_pool_size = m_params->GetParameterValue<int>("nthreads");
            }
        }

        m_params->SetDefaultParameter("jana:event_pool_size", m_event_pool_size);
        m_params->SetDefaultParameter("jana:limit_total_events_in_flight", m_limit_total_events_in_flight);
        m_params->SetDefaultParameter("jana:event_queue_threshold", m_event_queue_threshold);
        m_params->SetDefaultParameter("jana:event_source_chunksize", m_event_source_chunksize);
        m_params->SetDefaultParameter("jana:event_processor_chunksize", m_event_processor_chunksize);
        m_params->SetDefaultParameter("jana:enable_stealing", m_enable_stealing);
        m_params->SetDefaultParameter("jana:affinity", m_affinity);
        m_params->SetDefaultParameter("jana:locality", m_locality);
        m_params->SetDefaultParameter("RECORD_CALL_STACK", m_enable_call_graph_recording);

    };

    inline std::shared_ptr<JArrowTopology> create_empty() {
        m_topology = std::make_shared<JArrowTopology>();
        m_topology->component_manager = m_components;  // Ensure the lifespan of the component manager exceeds that of the topology
        m_topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(m_affinity),
                                       static_cast<JProcessorMapping::LocalityStrategy>(m_locality));

        m_topology->event_pool = std::make_shared<JEventPool>(m_components,
                                                              m_event_pool_size,
                                                              m_location_count,
                                                              m_limit_total_events_in_flight);
        return m_topology;

    }
/*
    inline void add_eventsource_arrows() {

    }

    inline void add_eventproc_arrow() {

    }

    template<typename S, typename T>
    void add_subevent_arrows() {

    }

    inline void add_blockeventsource_arrow() {

    }
*/

    inline std::shared_ptr<JArrowTopology> create_default_topology() {

        create_empty();

        auto queue = new EventQueue(m_event_queue_threshold, m_topology->mapping.get_loc_count(), m_enable_stealing);
        m_topology->queues.push_back(queue);

        for (auto src: m_components->get_evt_srces()) {

            // create arrow for each source. Don't open until arrow.activate() called
            JArrow *arrow = new JEventSourceArrow(src->GetName(), src, queue, m_topology->event_pool);
            arrow->set_backoff_tries(0);
            m_topology->arrows.push_back(arrow);
            m_topology->sources.push_back(arrow);
            arrow->set_chunksize(m_event_source_chunksize);
        }

        auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, m_topology->event_pool);
        proc_arrow->set_chunksize(m_event_processor_chunksize);
        m_topology->arrows.push_back(proc_arrow);

        for (auto proc: m_components->get_evt_procs()) {
            proc_arrow->add_processor(proc);
        }
        for (auto src_arrow : m_topology->sources) {
            src_arrow->attach(proc_arrow);
        }
        m_topology->sinks.push_back(proc_arrow);
        return m_topology;
    }

};


#endif //JANA2_JTOPOLOGYBUILDER_H
