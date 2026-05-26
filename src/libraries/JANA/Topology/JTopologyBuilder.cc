
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JTopologyBuilder.h"

#include <string>
#include <vector>

#include "JANA/Topology/JArrow.h"
#include "JANA/Utils/JEventLevel.h"
#include "JSourceArrow.h"
#include "JMultilevelSourceArrow.h"
#include "JMapArrow.h"
#include "JTapArrow.h"
#include "JUnfoldArrow.h"
#include "JFoldArrow.h"
#include <JANA/JEventProcessor.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Utils/JTablePrinter.h>


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

void JTopologyBuilder::AddArrow(JArrow* arrow) {
    arrows.push_back(arrow);
    arrow->SetId(arrows.size()-1);
    auto it = arrow_lookup.find(arrow->GetName());
    if (it != arrow_lookup.end()) {
        throw JException("AddArrow(): Arrow with name '%s' has already been added", arrow->GetName().c_str());
    }
    arrow_lookup[arrow->GetName()] = arrow;
}

JArrow* JTopologyBuilder::GetArrow(const std::string& arrow_name) {
    auto it = arrow_lookup.find(arrow_name);
    if (it == arrow_lookup.end()) {
        return nullptr;
    }
    return it->second;
}

JEventPool* JTopologyBuilder::GetOrCreatePool(JEventLevel level) {
    auto pool_it = pool_lookup.find(level);
    if (pool_it != pool_lookup.end()) {
        return pool_it->second;
    }
    else {
        auto* pool = new JEventPool(m_components, m_max_inflight_events[level], m_location_count, level);
        pools.push_back(pool);
        pool_lookup[level] = pool;
        return pool;
    }
}

void JTopologyBuilder::ConnectPool(std::string arrow_name, std::string port_name, JEventLevel level) {
    auto& arrow = *arrow_lookup.at(arrow_name);
    auto port_index = arrow.GetPortIndex(port_name);
    auto& port = arrow.GetPort(port_index);
    auto* pool = GetOrCreatePool(level);
    port.Attach(pool);
}

void JTopologyBuilder::ConnectPool(JEventLevel upstream_level, JEventLevel downstream_level) {
    auto* upstream_pool = GetOrCreatePool(upstream_level);
    auto* downstream_pool = GetOrCreatePool(downstream_level);
    upstream_pool->AttachForwardingPool(downstream_pool);
}

void JTopologyBuilder::ConnectQueue(std::string upstream_arrow_name, 
                                    std::string upstream_port_name,
                                    std::string downstream_arrow_name, 
                                    std::string downstream_port_name) {

    auto& upstream_arrow = *arrow_lookup.at(upstream_arrow_name);
    auto upstream_port_id = upstream_arrow.GetPortIndex(upstream_port_name);
    auto& downstream_arrow = *arrow_lookup.at(downstream_arrow_name);
    auto downstream_port_id = downstream_arrow.GetPortIndex(downstream_port_name);

    Connect(&upstream_arrow, upstream_port_id,
            &downstream_arrow, downstream_port_id);
}

std::string JTopologyBuilder::PrintTopology() {
    JTablePrinter t;
    t.AddColumn("Arrow", JTablePrinter::Justify::Left, 0);
    t.AddColumn("Parallel", JTablePrinter::Justify::Center, 0);
    t.AddColumn("Port", JTablePrinter::Justify::Left, 0);
    t.AddColumn("Place", JTablePrinter::Justify::Left, 0);
    t.AddColumn("ID", JTablePrinter::Justify::Left, 0);
    t.AddColumn("Order", JTablePrinter::Justify::Left, 0);

    // Build index lookup for queues
    int i = 0;
    std::map<void*, int> lookup;
    // Build index lookup for pools
    for (JEventPool* pool : pools) {
        lookup[pool] = i;
        i += 1;
    }
    for (JEventQueue* queue : queues) {
        lookup[queue] = i;
        i += 1;
    }
    // Build table

    bool show_row = true;
    
    for (JArrow* arrow : arrows) {

        show_row = true;
        for (auto& port : arrow->m_ports) {
            if (show_row) {
                t | arrow->GetName();
                t | arrow->IsParallel();
                show_row = false;
            }
            else {
                t | "" | "" ;
            }
            auto place_index = lookup[(port->GetQueue()!=nullptr) ? (void*) port->GetQueue() : (void*) port->GetPool()];

            t | port->GetName();
            t | ((port->GetQueue() != nullptr) ? "Queue ": "Pool");
            t | place_index;
            if (port->GetEnforcesOrdering() && port->GetEstablishesOrdering()) {
                t | "Both";
            }
            else if (port->GetEnforcesOrdering()) {
                t | "Enf";
            }
            else if (port->GetEstablishesOrdering()) {
                t | "Est";
            }
            else {
                t | "";
            }
        }
    }
    return t.Render();
}


/// set_cofigure_fn lets the user provide a lambda that sets up a topology after all components have been loaded.
/// It provides an 'empty' JArrowTopology which has been furnished with a pointer to the JComponentManager, the JEventPool,
/// and the JProcessorMapping (in case you care about NUMA details). However, it does not contain any queues or arrows.
/// You have to furnish those yourself.
void JTopologyBuilder::SetConfigureFn(std::function<void(JTopologyBuilder&, JComponentManager&)> configure_fn) {
    m_configure_topology = std::move(configure_fn);
}

void JTopologyBuilder::CreateTopology() {
    mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(m_affinity),
                       static_cast<JProcessorMapping::LocalityStrategy>(m_locality));

    if (m_configure_topology) {
        m_configure_topology(*this, *m_components);
        LOG_INFO(GetLogger()) << "Using custom topology configuration function" << LOG_END;
    }
    else {
        CreateTopologyFromScratch();
    }
    for (auto* arrow : arrows) {
        arrow->SetLogger(GetLogger());
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
    LOG_INFO(GetLogger()) << "Arrow topology is:\n" << PrintTopology() << LOG_END;
}

void JTopologyBuilder::CreateTopologyFromScratch() {

    enum class Column { Source, UnfoldAbove, BatchBefore, UnfoldBelow, FoldBelow, BatchAfter, Tap, FoldAbove};
    std::vector<Column> columns = { Column::Source, Column::UnfoldAbove, Column::BatchBefore, Column::UnfoldBelow, Column::FoldBelow, Column::BatchAfter, Column::Tap, Column::FoldAbove};
    struct Cell {
        JArrow* start = nullptr;
        JArrow* end = nullptr;
    };

    std::map<std::pair<JEventLevel, Column>, Cell> grid;

    // -----------------------------
    // Phase 1: Iterate over all components, adding the corresponding arrows to the grid
    // -----------------------------

    int map_counter = 1;
    std::set<JEventLevel> levels_present;

    // Place all sources on grid
    // -----------------------------
    std::map<JEventLevel, std::vector<JEventSource*>> sources;
    for (JEventSource* source : m_components->GetSources()) {
        if (source->IsEnabled()) {
            sources[source->GetLevel()].push_back(source);
            levels_present.insert(source->GetLevel());
        }
    }
    for (auto& it : sources) {
        auto level = it.first;
        auto level_str = toString(level);
        bool need_map = false;
        bool need_multi_arrow = false;
        for (auto* source : it.second) {
            need_map |= source->IsProcessParallelEnabled();
            need_multi_arrow |= (source->GetParentLevels().size() > 0);
        }

        if (need_multi_arrow && sources.size() > 1) {
            throw JException("Multiple multilevel JEventSources not supported yet");
        }

        JArrow* src_arrow;
        if (need_multi_arrow) {
            src_arrow = new JMultilevelSourceArrow(level_str+"MultiSource", it.second.at(0));
            // Add parent levels now. Child level is added further below.
            for (auto parent_level: it.second.at(0)->GetParentLevels()) {
                levels_present.insert(parent_level);
                grid[{parent_level, Column::Source}] = {src_arrow, src_arrow};
            }
        }
        else {
            src_arrow = new JSourceArrow(level_str+"Source", level, it.second);
        }
        AddArrow(src_arrow);

        if (need_map) {
            auto* map_arrow = new JMapArrow(toString(level)+"Map"+std::to_string(map_counter++), level);
            map_arrow->SetParallelSource(true);
            AddArrow(map_arrow);
            Connect(src_arrow, 1, map_arrow, 0);
            grid[{level, Column::Source}] = {src_arrow, map_arrow};
        }
        else {
            grid[{level, Column::Source}] = {src_arrow, src_arrow};
        }
    }

    // Place all unfolders on grid
    // -----------------------------
    for (auto* unfolder: m_components->GetUnfolders()) {

        if (!unfolder->IsEnabled()) continue;

        // Create unfold arrow
        // Publish at _each_ grid location
        auto parent_level = unfolder->GetLevel();
        auto child_level = unfolder->GetChildLevel();
        levels_present.insert(parent_level);
        levels_present.insert(child_level);

        auto* map_arrow = new JMapArrow(toString(parent_level)+"Map"+std::to_string(map_counter++), parent_level);
        auto* unfold_arrow = new JUnfoldArrow(toString(child_level)+"Unfold", unfolder);
        map_arrow->AddUnfolder(unfolder);
        AddArrow(map_arrow);
        AddArrow(unfold_arrow);
        Connect(map_arrow, map_arrow->EVENT_OUT, unfold_arrow, unfold_arrow->PARENT_IN);

        if (grid.find({parent_level, Column::UnfoldBelow}) != grid.end()) {
            throw JException("Only one unfolder allowed for parent level=%s", toString(parent_level).c_str());
        }
        if (grid.find({child_level, Column::UnfoldAbove}) != grid.end()) {
            throw JException("Only one unfolder allowed for child level=%s", toString(child_level).c_str());
        }
        grid[{parent_level, Column::UnfoldBelow}] = {map_arrow, unfold_arrow};
        grid[{child_level, Column::UnfoldAbove}] = {unfold_arrow, unfold_arrow};
    }

    // Place all folders on grid
    // -----------------------------
    for (auto* folder: m_components->GetFolders()) {

        if (!folder->IsEnabled()) continue;

        // Create unfold arrow
        // Publish at _each_ grid location
        auto parent_level = folder->GetLevel();
        auto child_level = folder->GetChildLevel();
        levels_present.insert(parent_level);
        levels_present.insert(child_level);

        auto* map_arrow = new JMapArrow(toString(child_level)+"Map"+std::to_string(map_counter++), parent_level);
        auto* fold_arrow = new JFoldArrow(toString(parent_level)+"Fold", parent_level, child_level);
        fold_arrow->SetFolder(folder);
        map_arrow->AddFolder(folder);
        AddArrow(map_arrow);
        AddArrow(fold_arrow);
        Connect(map_arrow, map_arrow->EVENT_OUT, fold_arrow, fold_arrow->CHILD_IN);

        if (grid.find({parent_level, Column::FoldBelow}) != grid.end()) {
            throw JException("Only one folder allowed for parent level=%s", toString(parent_level).c_str());
        }
        if (grid.find({child_level, Column::FoldAbove}) != grid.end()) {
            throw JException("Only one folder allowed for child level=%s", toString(child_level).c_str());
        }
        grid[{parent_level, Column::FoldBelow}] = {fold_arrow, fold_arrow};
        grid[{child_level, Column::FoldAbove}] = {map_arrow, fold_arrow};
    }

    // Place all processors on grid
    // -----------------------------
    std::map<JEventLevel, std::vector<JEventProcessor*>> mappable_processors;
    std::map<JEventLevel, std::vector<JEventProcessor*>> tappable_processors;
    for (auto* proc : m_components->GetProcessors()) {
        if (proc->IsEnabled()) {
            levels_present.insert(proc->GetLevel());
            if (proc->GetCallbackStyle() == JEventProcessor::CallbackStyle::LegacyMode && proc->IsOrderingEnabled()) {
                throw JException("%s: Ordering can only be used with non-legacy JEventProcessors", proc->GetTypeName().c_str());
            }
            mappable_processors[proc->GetLevel()].push_back(proc);
            if (proc->GetCallbackStyle() != JEventProcessor::CallbackStyle::LegacyMode) {
                tappable_processors[proc->GetLevel()].push_back(proc);
            }
        }
    }
    for (auto it : mappable_processors) {
        auto level = it.first;
        auto level_str = toString(level);
        auto* map_arrow = new JMapArrow(level_str+"Map"+std::to_string(map_counter++), level);
        for (JEventProcessor* proc : it.second) {
            map_arrow->AddProcessor(proc);
        }
        AddArrow(map_arrow);

        auto tappable_procs_it = tappable_processors.find(level);
        if (tappable_procs_it != tappable_processors.end()) {
            JArrow* first_tap_arrow = nullptr;
            JArrow* last_tap_arrow = nullptr;
            std::tie(first_tap_arrow, last_tap_arrow) = CreateTapChain(it.second, level_str);
            Connect(map_arrow, map_arrow->EVENT_OUT, first_tap_arrow, JTapArrow::EVENT_IN);
            grid[{level, Column::Tap}] = {map_arrow, last_tap_arrow};
        }
        else {
            // ONLY legacy processors, no tap chain
            grid[{level, Column::Tap}] = {map_arrow, map_arrow};
        }
    }

    // -----------------------------
    // Phase 2: Iterate over all rows and all adjacent occupied column pairs, wiring horizontally
    // -----------------------------

    for (JEventLevel level : levels_present) {

        auto* pool = GetOrCreatePool(level);
        JArrow* last_arrow = nullptr;
        for (auto column : columns) {
            auto it = grid.find({level, column});
            if (it == grid.end()) { continue; }

            JArrow* current_arrow = it->second.start;
            if (last_arrow == nullptr) {
                // This is the first arrow we've found, so connect the pool here
                auto port_index = current_arrow->GetPortIndex(level, JArrow::PortDirection::In);
                current_arrow->GetPort(port_index).Attach(pool);
            }
            else {
                Connect(last_arrow,
                        last_arrow->GetPortIndex(level, JArrow::PortDirection::Out),
                        current_arrow, 
                        current_arrow->GetPortIndex(level, JArrow::PortDirection::In));
            }
            last_arrow = it->second.end;

        }
        // Connect last_arrow to pool
        auto port_index = last_arrow->GetPortIndex(level, JArrow::PortDirection::Out);
        if (level == JEventLevel::PhysicsEvent) {
            last_arrow->SetIsSink(true);
        }
        last_arrow->GetPort(port_index).Attach(pool);
    }

    // -----------------------------
    // Phase 3: Traverse event hierarchy and attach levels accordingly
    // -----------------------------
    // Because we haven't fully implemented the event hierarchy yet, let's just go with the fully connected graph

    for (auto outer_level : levels_present) {
        for (auto inner_level : levels_present) {
            if (outer_level != inner_level) {
                ConnectPool(outer_level, inner_level);
            }
        }
    }
}


void JTopologyBuilder::Init() {

    m_components = GetApplication()->GetService<JComponentManager>();

    // We default event pool size to be equal to nthreads
    // We parse the 'nthreads' parameter two different ways for backwards compatibility.
    size_t nthreads = 1;
    if (m_params->Exists("nthreads")) {
        if (m_params->GetParameterValue<std::string>("nthreads") == "Ncores") {
            nthreads = JCpuInfo::GetNumCpus();
        } else {
            nthreads = m_params->GetParameterValue<int>("nthreads");
        }
    }

    m_max_inflight_events[JEventLevel::Run] = m_params->RegisterParameter("jana:max_inflight_runs", nthreads,
                                "The number of runs which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Subrun] = m_params->RegisterParameter("jana:max_inflight_subruns", nthreads,
                                "The number of subruns which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Timeslice] = m_params->RegisterParameter("jana:max_inflight_timeslices", nthreads,
                                "The number of timeslices which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Block] = m_params->RegisterParameter("jana:max_inflight_blocks", nthreads,
                                "The number of blocks which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::SlowControls] = m_params->RegisterParameter("jana:max_inflight_slowcontrols", nthreads,
                                "The number of slow control events which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::PhysicsEvent] = m_params->RegisterParameter("jana:max_inflight_events", nthreads,
                                "The number of physics events which may be in-flight at once. Should be at least `nthreads` to prevent starvation; more gives better load balancing.");

    m_max_inflight_events[JEventLevel::Subevent] = m_params->RegisterParameter("jana:max_inflight_subevents", 4*nthreads,
                                "The number of subevents which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Task] = m_params->RegisterParameter("jana:max_inflight_tasks", 8*nthreads,
                                "The number of tasks which may be in-flight at once.");

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


void JTopologyBuilder::Connect(JArrow* upstream, size_t upstream_port_id, JArrow* downstream, size_t downstream_port_id) {

    JArrow::Port& upstream_port = upstream->GetPort(upstream_port_id);
    JArrow::Port& downstream_port = downstream->GetPort(downstream_port_id);

    LOG_DEBUG(GetLogger()) << "Connecting arrows: " << upstream->GetName() << ":" << upstream_port.GetName() << " --> " << downstream->GetName() << ":" << downstream_port.GetName();

    // Enforce that multiple upstreams can share a downstream, but not vice versa
    if (upstream_port.GetQueue() != nullptr) {
        throw JException("Upstream port '%s' on arrow '%s' already has a queue", upstream_port.GetName().c_str(), upstream->GetName().c_str());
    }

    // Enforce that any event levels that are produced upstream must be accepted downstream
    for (auto level: upstream_port.GetLevels()) {
        bool level_found = false;
        for (auto downstream_level : downstream_port.GetLevels()) {
            if (downstream_level == level) {
                level_found = true;
                break;
            }
        }
        if (!level_found) {
            LOG_FATAL(GetLogger()) << "Level " << toString(level) << " produced upstream but not accepted downstream: "
                << upstream->GetName() << ":" << upstream_port.GetName() << " --> " << downstream->GetName() << ":" << downstream_port.GetName();
            throw JException("Level produced upstream but not accepted downstream");
        }
    }

    JEventQueue* queue = nullptr;
    if (downstream_port.GetQueue() != nullptr) {
        queue = downstream_port.GetQueue();
    }
    else {
        // Create new queue
        size_t queue_capacity = 0;
        for (auto level : downstream_port.GetLevels()) {
            queue_capacity += m_max_inflight_events[level];
        }
        queue = new JEventQueue(queue_capacity, mapping.get_loc_count());
        queues.push_back(queue);
        downstream_port.Attach(queue);
    }

    upstream_port.Attach(queue);

    if (downstream_port.GetEnforcesOrdering()) {
        queue->SetEnforcesOrdering();
    }
    if (upstream_port.GetEstablishesOrdering()) {
        queue->SetEstablishesOrdering(true);
    }
}


std::pair<JTapArrow*, JTapArrow*> JTopologyBuilder::CreateTapChain(std::vector<JEventProcessor*>& procs, std::string level) {

    JTapArrow* first = nullptr;
    JTapArrow* last = nullptr;

    int i=1;
    for (JEventProcessor* proc : procs) {
        std::string arrow_name = level + "Tap";
        if (procs.size() > 1) {
            arrow_name += std::to_string(i++);
        }
        JTapArrow* current = new JTapArrow(arrow_name, proc->GetLevel());
        current->AddProcessor(proc);
        AddArrow(current);
        if (first == nullptr) {
            first = current;
        }
        if (last != nullptr) {
            Connect(last, JTapArrow::EVENT_OUT, current, JTapArrow::EVENT_IN);
        }
        last = current;
    }
    return {first, last};
}


