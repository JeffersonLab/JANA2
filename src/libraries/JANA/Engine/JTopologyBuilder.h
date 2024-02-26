
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JTOPOLOGYBUILDER_H
#define JANA2_JTOPOLOGYBUILDER_H

#include <JANA/Engine/JArrowTopology.h>

#include "JEventSourceArrow.h"
#include "JEventProcessorArrow.h"
#include "JEventMapArrow.h"
#include "JUnfoldArrow.h"
#include "JFoldArrow.h"

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
            m_topology = std::make_shared<JArrowTopology>();
            m_topology->component_manager = m_components;  // Ensure the lifespan of the component manager exceeds that of the topology
            m_topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(m_affinity),
                                        static_cast<JProcessorMapping::LocalityStrategy>(m_locality));

            attach_top_level(JEventLevel::Run);
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

        m_topology->event_pool = new JEventPool(m_components,
                                                m_event_pool_size,
                                                m_location_count,
                                                m_limit_total_events_in_flight);
        m_topology->event_pool->init();
        return m_topology;

    }

    void attach_lower_level(JEventLevel current_level, JArrow* parent_unfolder, JArrow* parent_folder) {

        // Attach unfolders, folders, processors
        
        JEventPool* pool = new JEventPool(m_components,
                                          m_event_pool_size,
                                          m_location_count,
                                          m_limit_total_events_in_flight, 
                                          current_level);
        pool->init();
        m_topology->pools.push_back(pool); // Establishes ownership
        
        std::vector<JEventUnfolder*> unfolders_at_level;
        for (JEventUnfolder* unfolder : m_components->get_unfolders()) {
            if (unfolder->GetLevel() == current_level) {
                unfolders_at_level.push_back(unfolder);
            }
        }
        if (unfolders_at_level.size() == 0) {
            // No unfolders, so this is the last level
            // skip_lower_level(next_level(current_level));
        }
        else {
            // TODO:: Attach preprocess, unfolder arrow, folder arrow
            // attach_lower_level(next_level(current_level));
        }


    }

    void attach_top_level(JEventLevel current_level) {


        std::vector<JEventSource*> sources_at_level;
        for (JEventSource* source : m_components->get_evt_srces()) {
            if (source->GetLevel() == current_level) {
                sources_at_level.push_back(source);
            }
        }
        if (sources_at_level.size() == 0) {
            // Skip level entirely for now. Consider eventually supporting 
            // folding low levels into higher levels without corresponding unfold
            LOG_TRACE(m_arrow_logger) << "JTopologyBuilder: No sources found at level " << current_level << ", skipping" << LOG_END;
            JEventLevel next = next_level(current_level);
            if (next == JEventLevel::None) {
                LOG_WARN(m_arrow_logger) << "No sources found: Processing topology will be empty." << LOG_END;
                return;
            }
            return attach_top_level(next);
        }
        LOG_DEBUG(m_arrow_logger) << "JTopologyBuilder: Attaching components at level " << current_level << LOG_END;

        // We've now found our top level. No matter what, we need an event pool for this level
        JEventPool* pool_at_level = new JEventPool(m_components,
                                                   m_event_pool_size,
                                                   m_location_count,
                                                   m_limit_total_events_in_flight, 
                                                   current_level);
        pool_at_level->init();
        m_topology->pools.push_back(pool_at_level); // Hand over ownership of the pool to the topology

        // There are two possibilities at this point:
        // a. This is the only level, in which case we wire up the arrows and exit
        // b. We have an unfolder/folder pair, in which case we wire everything up, and then recursively attach_lower_level().
        // We use the presence of an unfolder as our test for whether or not a lower level should be included. This is because
        // the folder might be trivial and hence omitted by the user. (Note that some folder is always needed in order to return 
        // the higher-level event to the pool). 
        // The user always needs to provide an unfolder because I can't think of a trivial unfolder that would be useful.

        std::vector<JEventUnfolder*> unfolders_at_level;
        for (JEventUnfolder* unfolder : m_components->get_unfolders()) {
            if (unfolder->GetLevel() == current_level) {
                unfolders_at_level.push_back(unfolder);
            }
        }

        std::vector<JEventProcessor*> procs_at_level;
        for (JEventProcessor* proc : m_components->get_evt_procs()) {
            if (proc->GetLevel() == current_level) {
                procs_at_level.push_back(proc);
            }
        }

        if (unfolders_at_level.size() == 0) {
            // No unfolders, so this is the only level
            // Attach the source to the map/tap just like before
            //
            // We might want to print a friendly warning message communicating why any lower-level
            // components are being ignored, like so:
            // skip_lower_level(next_level(current_level));

            LOG_DEBUG(m_arrow_logger) << "JTopologyBuilder: No unfolders found at level " << current_level << ", finishing here." << LOG_END;

            auto queue = new EventQueue(m_event_queue_threshold, m_topology->mapping.get_loc_count(), m_enable_stealing);
            m_topology->queues.push_back(queue);

            auto* src_arrow = new JEventSourceArrow("sources", sources_at_level, queue, pool_at_level);
            m_topology->arrows.push_back(src_arrow);
            src_arrow->set_chunksize(m_event_source_chunksize);
            src_arrow->set_logger(m_arrow_logger);

            auto* proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, pool_at_level);
            m_topology->arrows.push_back(proc_arrow);
            proc_arrow->set_chunksize(m_event_processor_chunksize);
            proc_arrow->set_logger(m_arrow_logger);

            for (auto proc: procs_at_level) {
                proc_arrow->add_processor(proc);
            }
            src_arrow->attach(proc_arrow);
        }
        else if (unfolders_at_level.size() != 1) {
            throw JException("At most one unfolder must be provided for each level in the event hierarchy!");
        }
        else {
            // Have our unfolder, so we need to connect our source arrow
            // to our unfolder (and maybe preprocessor) arrows
            // Here we attach the source to the map to the unfolder
            // Also the folder to the tap
            // Then we pass the unfolder and folder to the attach_lower_level so it can hook those up as well
            // attach_lower_level(next_level(current_level));
            
            auto q1 = new EventQueue(m_event_queue_threshold, m_topology->mapping.get_loc_count(), m_enable_stealing);
            auto q2 = new EventQueue(m_event_queue_threshold, m_topology->mapping.get_loc_count(), m_enable_stealing);

            m_topology->queues.push_back(q1);
            m_topology->queues.push_back(q2);

            auto *src_arrow = new JEventSourceArrow("sources", sources_at_level, q1, pool_at_level);
            m_topology->arrows.push_back(src_arrow);
            src_arrow->set_chunksize(m_event_source_chunksize);
            src_arrow->set_logger(m_arrow_logger);

            auto *map_arrow = new JEventMapArrow("maps", q1, q2);;
            m_topology->arrows.push_back(map_arrow);
            map_arrow->set_chunksize(m_event_source_chunksize);
            map_arrow->set_logger(m_arrow_logger);
            src_arrow->attach(map_arrow);

            auto *unfold_arrow = new JUnfoldArrow("unfold", unfolders_at_level[0], q2, nullptr, nullptr);
            m_topology->arrows.push_back(unfold_arrow);
            unfold_arrow->set_chunksize(m_event_source_chunksize);
            unfold_arrow->set_logger(m_arrow_logger);
            map_arrow->attach(unfold_arrow);

            auto *fold_arrow = new JFoldArrow("fold", current_level, nullptr, pool_at_level, nullptr);
            // TODO: Support user-provided folders
            m_topology->arrows.push_back(fold_arrow);
            fold_arrow->set_chunksize(m_event_source_chunksize);
            fold_arrow->set_logger(m_arrow_logger);

            if (procs_at_level.size() != 0) {

                auto q3 = new EventQueue(m_event_queue_threshold, m_topology->mapping.get_loc_count(), m_enable_stealing);
                m_topology->queues.push_back(q3);

                auto* proc_arrow = new JEventProcessorArrow("processors", q3, nullptr, pool_at_level);
                m_topology->arrows.push_back(proc_arrow);
                proc_arrow->set_chunksize(m_event_processor_chunksize);
                proc_arrow->set_logger(m_arrow_logger);

                for (auto proc: procs_at_level) {
                    proc_arrow->add_processor(proc);
                }

                // TODO: Redirect fold_arrow to output to q3 instead of pool
                fold_arrow->attach(proc_arrow);
            }
        }
    }



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
        m_topology->arrows.push_back(arrow);
        arrow->set_chunksize(m_event_source_chunksize);
        arrow->set_logger(m_arrow_logger);


        auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, m_topology->event_pool);
        proc_arrow->set_chunksize(m_event_processor_chunksize);
        proc_arrow->set_logger(m_arrow_logger);
        m_topology->arrows.push_back(proc_arrow);

        for (auto proc: m_components->get_evt_procs()) {
            proc_arrow->add_processor(proc);
        }
        arrow->attach(proc_arrow);
        return m_topology;
    }




};


#endif //JANA2_JTOPOLOGYBUILDER_H
