
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JTopologyBuilder.h"

#include "JEventSourceArrow.h"
#include "JEventMapArrow.h"
#include "JEventTapArrow.h"
#include "JUnfoldArrow.h"
#include "JFoldArrow.h"
#include <JANA/JEventProcessor.h>
#include <JANA/Utils/JTablePrinter.h>
#include <string>
#include <vector>


JTopologyBuilder::JTopologyBuilder() {
    SetPrefix("jana");
}

JTopologyBuilder::~JTopologyBuilder() {
    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto queue : queues) {
        delete queue;
    }
    for (auto pool : pools) {
        delete pool;
    }
}

std::string JTopologyBuilder::print_topology() {
    JTablePrinter t;
    t.AddColumn("Arrow", JTablePrinter::Justify::Left, 0);
    t.AddColumn("Parallel", JTablePrinter::Justify::Center, 0);
    t.AddColumn("Direction", JTablePrinter::Justify::Left, 0);
    t.AddColumn("Place", JTablePrinter::Justify::Left, 0);
    t.AddColumn("ID", JTablePrinter::Justify::Left, 0);

    // Build index lookup for queues
    int i = 0;
    std::map<void*, int> lookup;
    for (JEventQueue* queue : queues) {
        lookup[queue] = i;
        i += 1;
    }
    // Build index lookup for pools
    for (JEventPool* pool : pools) {
        lookup[pool] = i;
        i += 1;
    }
    // Build table

    bool show_row = true;
    
    for (JArrow* arrow : arrows) {

        show_row = true;
        for (JArrow::Port& port : arrow->m_ports) {
            if (show_row) {
                t | arrow->get_name();
                t | arrow->is_parallel();
                show_row = false;
            }
            else {
                t | "" | "" ;
            }
            auto place_index = lookup[(port.queue!=nullptr) ? (void*) port.queue : (void*) port.pool];

            t | ((port.is_input) ? "Input ": "Output");
            t | ((port.queue != nullptr) ? "Queue ": "Pool");
            t | place_index;
        }
    }
    return t.Render();
}


/// set_cofigure_fn lets the user provide a lambda that sets up a topology after all components have been loaded.
/// It provides an 'empty' JArrowTopology which has been furnished with a pointer to the JComponentManager, the JEventPool,
/// and the JProcessorMapping (in case you care about NUMA details). However, it does not contain any queues or arrows.
/// You have to furnish those yourself.
void JTopologyBuilder::set_configure_fn(std::function<void(JTopologyBuilder&)> configure_fn) {
    m_configure_topology = std::move(configure_fn);
}

void JTopologyBuilder::create_topology() {
    mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(m_affinity),
                       static_cast<JProcessorMapping::LocalityStrategy>(m_locality));

    if (m_configure_topology) {
        m_configure_topology(*this);
        LOG_WARN(GetLogger()) << "Found custom topology configurator! Modified arrow topology is: \n" << print_topology() << LOG_END;
    }
    else {
        attach_level(JEventLevel::Run, nullptr, nullptr);
        LOG_INFO(GetLogger()) << "Arrow topology is:\n" << print_topology() << LOG_END;
    }
    for (auto* arrow : arrows) {
        arrow->set_logger(GetLogger());
    }

    // _Don't_ establish ordering if nobody needs it!
    // This hopefully prevents a small hit to NUMA performance
    // because we don't need to update next_event_index across NUMA nodes
    bool need_ordering = false;
    for (auto* queue : queues) {
        if (queue->GetEnforcesOrdering()) {
            need_ordering = true;
        }
    }
    if (!need_ordering) {
        for (auto* queue : queues) {
            queue->SetEstablishesOrdering(false);
        }
    }
}


void JTopologyBuilder::Init() {

    m_components = GetApplication()->GetService<JComponentManager>();

    // We default event pool size to be equal to nthreads
    // We parse the 'nthreads' parameter two different ways for backwards compatibility.
    if (m_params->Exists("nthreads")) {
        if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
            m_max_inflight_events = JCpuInfo::GetNumCpus();
        } else {
            m_max_inflight_events = m_params->GetParameterValue<int>("nthreads");
        }
    }

    m_params->SetDefaultParameter("jana:max_inflight_events", m_max_inflight_events,
                                    "The number of events which may be in-flight at once. Should be at least `nthreads` to prevent starvation; more gives better load balancing.")
            ->SetIsAdvanced(true);

    /*
    m_params->SetDefaultParameter("jana:enable_stealing", m_enable_stealing,
                                    "Enable work stealing. Improves load balancing when jana:locality != 0; otherwise does nothing.")
            ->SetIsAdvanced(true);
    */
    m_params->SetDefaultParameter("jana:affinity", m_affinity,
                                    "Constrain worker thread CPU affinity. 0=Let the OS decide. 1=Avoid extra memory movement at the expense of using hyperthreads. 2=Avoid hyperthreads at the expense of extra memory movement")
            ->SetIsAdvanced(true);
    m_params->SetDefaultParameter("jana:locality", m_locality,
                                    "Constrain memory locality. 0=No constraint. 1=Events stay on the same socket. 2=Events stay on the same NUMA domain. 3=Events stay on same core. 4=Events stay on same cpu/hyperthread.")
            ->SetIsAdvanced(true);
};


void JTopologyBuilder::connect(JArrow* upstream, size_t upstream_port_id, JArrow* downstream, size_t downstream_port_id) {

    JEventQueue* queue = nullptr;

    JArrow::Port& downstream_port = downstream->m_ports.at(downstream_port_id);
    if (downstream_port.queue != nullptr) {
        // If the queue already exists, use that!
        queue = downstream_port.queue;
    }
    else {
        // Create a new queue
        queue = new JEventQueue(m_max_inflight_events, mapping.get_loc_count());
        downstream_port.queue = queue;
        queues.push_back(queue);
    }
    downstream_port.pool = nullptr;
    if (downstream_port.enforces_ordering) {
        queue->SetEnforcesOrdering();
    }

    JArrow::Port& upstream_port = upstream->m_ports.at(upstream_port_id);
    upstream_port.queue = queue;
    if (upstream_port.establishes_ordering) {
        queue->SetEstablishesOrdering(true);
    }
    upstream_port.pool = nullptr;
}


void JTopologyBuilder::connect_to_first_available(JArrow* upstream, size_t upstream_port, std::vector<std::pair<JArrow*, size_t>> downstreams) {
    for (auto& [downstream, downstream_port_id] : downstreams) {
        if (downstream != nullptr) {
            connect(upstream, upstream_port, downstream, downstream_port_id);
            return;
        }
    }
}

std::pair<JEventTapArrow*, JEventTapArrow*> JTopologyBuilder::create_tap_chain(std::vector<JEventProcessor*>& procs, std::string level) {

    JEventTapArrow* first = nullptr;
    JEventTapArrow* last = nullptr;

    int i=1;
    std::string arrow_name = level + "Tap";
    for (JEventProcessor* proc : procs) {
        if (procs.size() > 1) {
            arrow_name += std::to_string(i++);
        }
        JEventTapArrow* current = new JEventTapArrow(arrow_name);
        current->add_processor(proc);
        arrows.push_back(current);
        if (first == nullptr) {
            first = current;
        }
        if (last != nullptr) {
            connect(last, JEventTapArrow::EVENT_OUT, current, JEventTapArrow::EVENT_IN);
        }
        last = current;
    }
    return {first, last};
}


void JTopologyBuilder::attach_level(JEventLevel current_level, JUnfoldArrow* parent_unfolder, JFoldArrow* parent_folder) {
    std::stringstream ss;
    ss << current_level;
    auto level_str = ss.str();

    // Find all event sources at this level
    std::vector<JEventSource*> sources_at_level;
    for (JEventSource* source : m_components->get_evt_srces()) {
        if (source->GetLevel() == current_level) {
            sources_at_level.push_back(source);
        }
    }
    
    // Find all unfolders at this level
    std::vector<JEventUnfolder*> unfolders_at_level;
    for (JEventUnfolder* unfolder : m_components->get_unfolders()) {
        if (unfolder->GetLevel() == current_level) {
            unfolders_at_level.push_back(unfolder);
        }
    }
    
    // Find all processors at this level
    std::vector<JEventProcessor*> mappable_procs_at_level;
    std::vector<JEventProcessor*> tappable_procs_at_level;

    for (JEventProcessor* proc : m_components->get_evt_procs()) {

        if (proc->GetLevel() == current_level) {

            // This may be a weird place to do it, but let's quickly validate that users aren't
            // trying to enable ordering on a legacy event processor. We don't do this in the constructor
            // because we don't want to put constraints on the order in which setters can be called, apart from "before Init()"

            if (proc->GetCallbackStyle() == JEventProcessor::CallbackStyle::LegacyMode && proc->IsOrderingEnabled()) {
                throw JException("%s: Ordering can only be used with non-legacy JEventProcessors", proc->GetTypeName().c_str());
            }

            mappable_procs_at_level.push_back(proc);
            if (proc->GetCallbackStyle() != JEventProcessor::CallbackStyle::LegacyMode) {
                tappable_procs_at_level.push_back(proc);
            }
        }
    }


    bool is_top_level = (parent_unfolder == nullptr);
    if (is_top_level && sources_at_level.size() == 0) {
        // Skip level entirely when no source is present.
        LOG_TRACE(GetLogger()) << "JTopologyBuilder: No sources found at level " << current_level << ", skipping" << LOG_END;
        JEventLevel next = next_level(current_level);
        if (next == JEventLevel::None) {
            LOG_WARN(GetLogger()) << "No sources found: Processing topology will be empty." << LOG_END;
            return;
        }
        return attach_level(next, nullptr, nullptr);
    }

    // Enforce constraints on what our builder will accept (at least for now)
    if (!is_top_level && !sources_at_level.empty()) {
        throw JException("Topology forbids event sources at lower event levels in the topology");
    }
    if ((parent_unfolder == nullptr && parent_folder != nullptr) || (parent_unfolder != nullptr && parent_folder == nullptr)) {
        throw JException("Topology requires matching unfolder/folder arrow pairs");
    }
    if (unfolders_at_level.size() > 1) {
        throw JException("Multiple JEventUnfolders provided for level %s", level_str.c_str());
    }
    // Another constraint is that the highest level of the topology has an event sources, but this is automatically handled by
    // the level-skipping logic above


    // Fill out arrow grid from components at this event level
    // --------------------------
    // 0. Pool
    // --------------------------
    LOG_INFO(GetLogger()) << "Creating event pool with level=" << current_level << " and size=" << m_max_inflight_events;
    JEventPool* pool_at_level = new JEventPool(m_components, m_max_inflight_events, m_location_count, current_level);
    pools.push_back(pool_at_level); // Hand over ownership of the pool to the topology
    LOG_INFO(GetLogger()) << "Created event pool with level=" << current_level << " and size=" << m_max_inflight_events;

    // --------------------------
    // 1. Source
    // --------------------------
    JEventSourceArrow* src_arrow = nullptr;
    bool need_source = !sources_at_level.empty();
    if (need_source) {
        src_arrow = new JEventSourceArrow(level_str+"Source", sources_at_level);
        src_arrow->attach(pool_at_level, JEventSourceArrow::EVENT_IN);
        src_arrow->attach(pool_at_level, JEventSourceArrow::EVENT_OUT);
        arrows.push_back(src_arrow);
    }

    // --------------------------
    // 2. Map1
    // --------------------------
    bool have_parallel_sources = false;
    for (JEventSource* source: sources_at_level) {
        have_parallel_sources |= source->IsProcessParallelEnabled();
    }
    bool have_unfolder = !unfolders_at_level.empty();
    JEventMapArrow* map1_arrow = nullptr;
    bool need_map1 = (have_parallel_sources || have_unfolder);

    if (need_map1) {
        map1_arrow = new JEventMapArrow(level_str+"Map1");
        for (JEventSource* source: sources_at_level) {
            if (source->IsProcessParallelEnabled()) {
                map1_arrow->add_source(source);
            }
        }
        for (JEventUnfolder* unf: unfolders_at_level) {
            map1_arrow->add_unfolder(unf);
        }
        map1_arrow->attach(pool_at_level, JEventMapArrow::EVENT_IN);
        map1_arrow->attach(pool_at_level, JEventMapArrow::EVENT_OUT);
        arrows.push_back(map1_arrow);
    }

    // --------------------------
    // 3. Unfold
    // --------------------------
    JUnfoldArrow* unfold_arrow = nullptr;
    bool need_unfold = have_unfolder;
    if (need_unfold) {
        unfold_arrow = new JUnfoldArrow(level_str+"Unfold", unfolders_at_level[0]);
        unfold_arrow->attach(pool_at_level, JUnfoldArrow::REJECTED_PARENT_OUT); 
        arrows.push_back(unfold_arrow);
    }

    // --------------------------
    // 4. Fold
    // --------------------------
    JFoldArrow* fold_arrow = nullptr;
    bool need_fold = have_unfolder;
    if(need_fold) {
        fold_arrow = new JFoldArrow(level_str+"Fold", current_level, unfolders_at_level[0]->GetChildLevel());
        arrows.push_back(fold_arrow);
        fold_arrow->attach(pool_at_level, JFoldArrow::PARENT_OUT); 
    }

    // --------------------------
    // 5. Map2
    // --------------------------
    JEventMapArrow* map2_arrow = nullptr;
    bool need_map2 = !mappable_procs_at_level.empty();
    if (need_map2) {
        map2_arrow = new JEventMapArrow(level_str+"Map2");
        for (JEventProcessor* proc : mappable_procs_at_level) {
            map2_arrow->add_processor(proc);
            map2_arrow->attach(pool_at_level, JEventMapArrow::EVENT_IN);
            map2_arrow->attach(pool_at_level, JEventMapArrow::EVENT_OUT);
        }
        arrows.push_back(map2_arrow);
    }

    // --------------------------
    // 6. Tap
    // --------------------------
    JEventTapArrow* first_tap_arrow = nullptr;
    JEventTapArrow* last_tap_arrow = nullptr;
    bool need_tap = !tappable_procs_at_level.empty();
    if (need_tap) {
        std::tie(first_tap_arrow, last_tap_arrow) = create_tap_chain(tappable_procs_at_level, level_str);
        first_tap_arrow->attach(pool_at_level, JEventTapArrow::EVENT_IN);
        last_tap_arrow->attach(pool_at_level, JEventTapArrow::EVENT_OUT);
    }


    // Now that we've set up our component grid, we can do wiring!
    // --------------------------
    // 1. Source
    // --------------------------
    if (parent_unfolder != nullptr) {
        parent_unfolder->attach(pool_at_level, JUnfoldArrow::CHILD_IN);
        connect_to_first_available(parent_unfolder, JUnfoldArrow::CHILD_OUT,
                                   {{map1_arrow, JEventMapArrow::EVENT_IN}, {unfold_arrow, JUnfoldArrow::PARENT_IN}, {map2_arrow, JEventMapArrow::EVENT_IN}, {first_tap_arrow, JEventTapArrow::EVENT_IN}, {parent_folder, JFoldArrow::CHILD_IN}});
    }
    if (src_arrow != nullptr) {
        connect_to_first_available(src_arrow, JEventSourceArrow::EVENT_OUT,
                                   {{map1_arrow, JEventMapArrow::EVENT_IN}, {unfold_arrow, JUnfoldArrow::PARENT_IN}, {map2_arrow, JEventMapArrow::EVENT_IN}, {first_tap_arrow, JEventTapArrow::EVENT_IN}, {parent_folder, JFoldArrow::CHILD_IN}});
    }
    if (map1_arrow != nullptr) {
        connect_to_first_available(map1_arrow, JEventMapArrow::EVENT_OUT,
                                   {{unfold_arrow, JUnfoldArrow::PARENT_IN}, {map2_arrow, JEventMapArrow::EVENT_IN}, {first_tap_arrow, JEventTapArrow::EVENT_IN}, {parent_folder, JFoldArrow::CHILD_IN}});
    }
    if (unfold_arrow != nullptr) {
        connect_to_first_available(unfold_arrow, JUnfoldArrow::REJECTED_PARENT_OUT,
                                   {{map2_arrow, JEventMapArrow::EVENT_IN}, {first_tap_arrow, JEventTapArrow::EVENT_IN}, {parent_folder, JFoldArrow::CHILD_IN}});
    }
    if (fold_arrow != nullptr) {
        connect_to_first_available(fold_arrow, JFoldArrow::CHILD_OUT,
                                   {{map2_arrow, JEventMapArrow::EVENT_IN}, {first_tap_arrow, JEventTapArrow::EVENT_IN}, {parent_folder, JFoldArrow::CHILD_IN}});
    }
    if (map2_arrow != nullptr) {
        connect_to_first_available(map2_arrow, JEventMapArrow::EVENT_OUT,
                                   {{first_tap_arrow, JEventTapArrow::EVENT_IN}, {parent_folder, JFoldArrow::CHILD_IN}});
    }
    if (last_tap_arrow != nullptr) {
        connect_to_first_available(last_tap_arrow, JEventTapArrow::EVENT_OUT,
                                   {{parent_folder, JFoldArrow::CHILD_IN}});
    }
    if (parent_folder != nullptr) {
        parent_folder->attach(pool_at_level, JFoldArrow::CHILD_OUT);
    }

    // Finally, we recur over lower levels!
    if (need_unfold) {
        auto next_level = unfolders_at_level[0]->GetChildLevel();
        attach_level(next_level, unfold_arrow, fold_arrow);
    }
    else {
        // This is the lowest level
        // TODO: Improve logic for determining event counts for multilevel topologies
        if (last_tap_arrow != nullptr) {
            last_tap_arrow->set_is_sink(true);
        }
        else if (map2_arrow != nullptr) {
            map2_arrow->set_is_sink(true);
        }
        else if (map1_arrow != nullptr) {
            map1_arrow->set_is_sink(true);
        }
        else if (src_arrow != nullptr) {
            src_arrow->set_is_sink(true);
        }
    }
}



