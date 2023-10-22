
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

public:

    JTopologyBuilder() = default;

    ~JTopologyBuilder() override = default;

    /// set allows the user to specify a topology directly. Note that this needs to be set before JApplication::Initialize
    /// gets called, which means that you won't be able to include components loaded from plugins. You probably want to use
    /// JTopologyBuilder::set_configure_fn instead, which does give you that access.
    inline void set(std::shared_ptr<JArrowTopology> topology) {
        m_topology = std::move(topology);
    }

    /// set_cofigure_fn lets the user provide a lambda that sets up a topology after all components have been loaded.
    /// It provides an 'empty' JArrowTopology which has been furnished with a pointer to the JComponentManager, the JEventPool,
    /// and the JProcessorMapping (in case you care about NUMA details). However, it does not contain any queues or arrows.
    /// You have to furnish those yourself.
    inline void set_configure_fn(std::function<std::shared_ptr<JArrowTopology>(std::shared_ptr<JArrowTopology>)> configure_fn) {
        m_configure_topology = std::move(configure_fn);
    }

    inline std::shared_ptr<JArrowTopology> get() {
        return m_topology;
    }

    inline std::shared_ptr<JArrowTopology> get_or_create() {
        if (m_topology != nullptr) {
            return m_topology;
        }
        else if (m_configure_topology) {
            m_topology = m_configure_topology(create_empty());
            return m_topology;
        }
        else {
            m_topology = create_default_topology();
            return m_topology;
        }
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

        m_params->SetDefaultParameter("jana:event_pool_size", m_event_pool_size,
                                      "Sets the initial size of the event pool. Having too few events starves the workers; having too many consumes memory and introduces overhead from extra factory initializations")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("jana:limit_total_events_in_flight", m_limit_total_events_in_flight,
                                      "Controls whether the event pool is allowed to automatically grow beyond jana:event_pool_size")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("jana:event_queue_threshold", m_event_queue_threshold,
                                      "Max number of events allowed on the main event queue. Higher => Better load balancing; Lower => Fewer events in flight")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("jana:event_source_chunksize", m_event_source_chunksize,
                                      "Max number of events that a JEventSource may enqueue at once. Higher => less queue contention; Lower => better load balancing")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("jana:event_processor_chunksize", m_event_processor_chunksize,
                                      "Max number of events that the JEventProcessors may dequeue at once. Higher => less queue contention; Lower => better load balancing")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("jana:enable_stealing", m_enable_stealing,
                                      "Enable work stealing. Improves load balancing when jana:locality != 0; otherwise does nothing.")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("jana:affinity", m_affinity,
                                      "Constrain worker thread CPU affinity. 0=Let the OS decide. 1=Avoid extra memory movement at the expense of using hyperthreads. 2=Avoid hyperthreads at the expense of extra memory movement")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("jana:locality", m_locality,
                                      "Constrain memory locality. 0=No constraint. 1=Events stay on the same socket. 2=Events stay on the same NUMA domain. 3=Events stay on same core. 4=Events stay on same cpu/hyperthread.")
                ->SetIsAdvanced(true);
        m_params->SetDefaultParameter("record_call_stack", m_enable_call_graph_recording,
                                      "Records a trace of who called each factory. Reduces performance but necessary for plugins such as janadot.")
                ->SetIsAdvanced(true);

        m_arrow_logger = sl->get<JLoggingService>()->get_logger("JArrow");
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

        // We generally want to assert that there is at least one event source (or block source, or any arrow in topology->sources, really),
        // as otherwise the topology fundamentally cannot do anything. We are not doing that here for two reasons:
        // 1. If a user provides a custom topology that is missing an event source, this won't catch it
        // 2. Oftentimes we want to call JApplication::Initialize() just to set up plugins and services, i.e. for testing.
        //    We don't want to force the user to create a dummy event source if they know they are never going to call JApplication::Run().

        // Create arrow for sources.
        JArrow *arrow = new JEventSourceArrow("sources", m_components->get_evt_srces(), queue, m_topology->event_pool);
        arrow->set_backoff_tries(0);
        m_topology->arrows.push_back(arrow);
        m_topology->sources.push_back(arrow);
        arrow->set_chunksize(m_event_source_chunksize);
        arrow->set_logger(m_arrow_logger);


        auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, m_topology->event_pool);
        proc_arrow->set_chunksize(m_event_processor_chunksize);
        proc_arrow->set_logger(m_arrow_logger);
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
