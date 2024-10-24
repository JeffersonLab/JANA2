
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


using Event = std::shared_ptr<JEvent>;
using EventQueue = JMailbox<Event*>;

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
    if (event_pool != nullptr) {
        delete event_pool;
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
    for (JQueue* queue : queues) {
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
        for (Place* place : arrow->m_places) {
            if (show_row) {
                t | arrow->get_name();
                t | arrow->is_parallel();
                show_row = false;
            }
            else {
                t | "" | "" ;
            }

            t | ((place->is_input) ? "Input ": "Output");
            t | ((place->is_queue) ? "Queue ": "Pool");
            t | lookup[place->place_ref];
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

    event_pool = new JEventPool(m_components, m_max_inflight_events, m_location_count);

    if (m_configure_topology) {
        m_configure_topology(*this);
        LOG_WARN(GetLogger()) << "Found custom topology configurator! Modified arrow topology is: \n" << print_topology() << LOG_END;
    }
    else {
        attach_level(JEventLevel::Run, nullptr, nullptr);
        LOG_INFO(GetLogger()) << "Arrow topology is:\n" << print_topology() << LOG_END;
    }
    int id=0;
    for (auto* queue : queues) {
        queue->set_logger(GetLogger());
        queue->set_id(id);
        id += 1;
    }
    for (auto* arrow : arrows) {
        arrow->set_logger(GetLogger());
    }
}


void JTopologyBuilder::acquire_services(JServiceLocator *sl) {

    m_components = sl->get<JComponentManager>();

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

    m_params->SetDefaultParameter("jana:enable_stealing", m_enable_stealing,
                                    "Enable work stealing. Improves load balancing when jana:locality != 0; otherwise does nothing.")
            ->SetIsAdvanced(true);
    m_params->SetDefaultParameter("jana:affinity", m_affinity,
                                    "Constrain worker thread CPU affinity. 0=Let the OS decide. 1=Avoid extra memory movement at the expense of using hyperthreads. 2=Avoid hyperthreads at the expense of extra memory movement")
            ->SetIsAdvanced(true);
    m_params->SetDefaultParameter("jana:locality", m_locality,
                                    "Constrain memory locality. 0=No constraint. 1=Events stay on the same socket. 2=Events stay on the same NUMA domain. 3=Events stay on same core. 4=Events stay on same cpu/hyperthread.")
            ->SetIsAdvanced(true);
};


void JTopologyBuilder::connect(JArrow* upstream, size_t up_index, JArrow* downstream, size_t down_index) {

    auto queue = new EventQueue(m_max_inflight_events, mapping.get_loc_count(), m_enable_stealing);
    queues.push_back(queue);

    size_t i = 0;
    for (Place* place : upstream->m_places) {
        if (!place->is_input) {
            if (i++ == up_index) {
                // Found the correct output
                place->is_queue = true;
                place->place_ref = queue;
            }
        }
    }
    i = 0;
    for (Place* place : downstream->m_places) {
        if (place->is_input) {
            if (i++ == down_index) {
                // Found the correct input
                place->is_queue = true;
                place->place_ref = queue;
            }
        }
    }
    upstream->attach(downstream);
}


void JTopologyBuilder::connect_to_first_available(JArrow* upstream, std::vector<JArrow*> downstreams) {
    for (JArrow* downstream : downstreams) {
        if (downstream != nullptr) {
            // Arrows at the same level all connect at index 0 (even the input for the parent JFoldArrow)
            connect(upstream, 0, downstream, 0);
            return;
        }
    }
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
    JEventPool* pool_at_level = new JEventPool(m_components, m_max_inflight_events, m_location_count, current_level);
    pools.push_back(pool_at_level); // Hand over ownership of the pool to the topology

    // --------------------------
    // 1. Source
    // --------------------------
    JEventSourceArrow* src_arrow = nullptr;
    bool need_source = !sources_at_level.empty();
    if (need_source) {
        src_arrow = new JEventSourceArrow(level_str+"Source", sources_at_level);
        src_arrow->set_input(pool_at_level);
        src_arrow->set_output(pool_at_level);
        arrows.push_back(src_arrow);
    }

    // --------------------------
    // 2. Map1
    // --------------------------
    bool have_parallel_sources = false;
    for (JEventSource* source: sources_at_level) {
        have_parallel_sources |= source->IsPreprocessEnabled();
    }
    bool have_unfolder = !unfolders_at_level.empty();
    JEventMapArrow* map1_arrow = nullptr;
    bool need_map1 = (have_parallel_sources || have_unfolder);

    if (need_map1) {
        map1_arrow = new JEventMapArrow(level_str+"Map1");
        for (JEventSource* source: sources_at_level) {
            if (source->IsPreprocessEnabled()) {
                map1_arrow->add_source(source);
            }
        }
        for (JEventUnfolder* unf: unfolders_at_level) {
            map1_arrow->add_unfolder(unf);
        }
        map1_arrow->set_input(pool_at_level);
        map1_arrow->set_output(pool_at_level);
        arrows.push_back(map1_arrow);
    }

    // --------------------------
    // 3. Unfold
    // --------------------------
    JUnfoldArrow* unfold_arrow = nullptr;
    bool need_unfold = have_unfolder;
    if (need_unfold) {
        unfold_arrow = new JUnfoldArrow(level_str+"Unfold", unfolders_at_level[0]);
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
        fold_arrow->attach_parent_out(pool_at_level);
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
            map2_arrow->set_input(pool_at_level);
            map2_arrow->set_output(pool_at_level);
        }
        arrows.push_back(map2_arrow);
    }

    // --------------------------
    // 6. Tap
    // --------------------------
    JEventTapArrow* tap_arrow = nullptr;
    bool need_tap = !tappable_procs_at_level.empty();
    if (need_tap) {
        tap_arrow = new JEventTapArrow(level_str+"Tap");
        for (JEventProcessor* proc : tappable_procs_at_level) {
            tap_arrow->add_processor(proc);
            tap_arrow->set_input(pool_at_level);
            tap_arrow->set_output(pool_at_level);
        }
        arrows.push_back(tap_arrow);
    }


    // Now that we've set up our component grid, we can do wiring!
    // --------------------------
    // 1. Source
    // --------------------------
    if (parent_unfolder != nullptr) {
        parent_unfolder->attach_child_in(pool_at_level);
        connect_to_first_available(parent_unfolder, {map1_arrow, unfold_arrow, map2_arrow, tap_arrow, parent_folder});
    }
    if (src_arrow != nullptr) {
        connect_to_first_available(src_arrow, {map1_arrow, unfold_arrow, map2_arrow, tap_arrow, parent_folder});
    }
    if (map1_arrow != nullptr) {
        connect_to_first_available(map1_arrow, {unfold_arrow, map2_arrow, tap_arrow, parent_folder});
    }
    if (fold_arrow != nullptr) {
        connect_to_first_available(fold_arrow, {map2_arrow, tap_arrow, parent_folder});
    }
    if (map2_arrow != nullptr) {
        connect_to_first_available(map2_arrow, {tap_arrow, parent_folder});
    }
    if (tap_arrow != nullptr) {
        connect_to_first_available(tap_arrow, {parent_folder});
    }
    if (parent_folder != nullptr) {
        parent_folder->attach_child_out(pool_at_level);
    }

    // Finally, we recur over lower levels!
    if (need_unfold) {
        auto next_level = unfolders_at_level[0]->GetChildLevel();
        attach_level(next_level, unfold_arrow, fold_arrow);
    }
    else {
        // This is the lowest level
        // TODO: Improve logic for determining event counts for multilevel topologies
        if (tap_arrow != nullptr) {
            tap_arrow->set_is_sink(true);
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



