
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JTopologyBuilder.h"

#include "JEventSourceArrow.h"
#include "JEventProcessorArrow.h"
#include "JEventMapArrow.h"
#include "JUnfoldArrow.h"
#include "JFoldArrow.h"
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
    for (JPoolBase* pool : pools) {
        lookup[pool] = i;
        i += 1;
    }
    // Build table

    bool show_row = true;
    
    for (JArrow* arrow : arrows) {

        show_row = true;
        for (PlaceRefBase* place : arrow->m_places) {
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

    event_pool = new JEventPool(m_components,
                                m_event_pool_size,
                                m_location_count,
                                m_limit_total_events_in_flight);
    event_pool->init();

    if (m_configure_topology) {
        m_configure_topology(*this);
        LOG_WARN(GetLogger()) << "Found custom topology configurator! Modified arrow topology is: \n" << print_topology() << LOG_END;
    }
    else {
        attach_top_level(JEventLevel::Run);
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


void JTopologyBuilder::attach_lower_level(JEventLevel current_level, JUnfoldArrow* parent_unfolder, JFoldArrow* parent_folder, bool found_sink) {

    std::stringstream ss;
    ss << current_level;

    LOG_DEBUG(GetLogger()) << "JTopologyBuilder: Attaching components at lower level = " << current_level << LOG_END;
    
    JEventPool* pool = new JEventPool(m_components,
                                        m_event_pool_size,
                                        m_location_count,
                                        m_limit_total_events_in_flight, 
                                        current_level);
    pool->init();
    pools.push_back(pool); // Transfers ownership


    std::vector<JEventSource*> sources_at_level;
    for (JEventSource* source : m_components->get_evt_srces()) {
        if (source->GetLevel() == current_level) {
            sources_at_level.push_back(source);
        }
    }
    std::vector<JEventProcessor*> procs_at_level;
    for (JEventProcessor* proc : m_components->get_evt_procs()) {
        if (proc->GetLevel() == current_level) {
            procs_at_level.push_back(proc);
        }
    }
    std::vector<JEventUnfolder*> unfolders_at_level;
    for (JEventUnfolder* unfolder : m_components->get_unfolders()) {
        if (unfolder->GetLevel() == current_level) {
            unfolders_at_level.push_back(unfolder);
        }
    }


    if (sources_at_level.size() != 0) {
        throw JException("Support for lower-level event sources coming soon!");
    }
    if (unfolders_at_level.size() != 0) {
        throw JException("Support for lower-level event unfolders coming soon!");
    }
    if (procs_at_level.size() == 0) {
        throw JException("For now we require you to provide at least one JEventProcessor");
    }

    auto q1 = new EventQueue(m_event_queue_threshold, mapping.get_loc_count(), m_enable_stealing);
    queues.push_back(q1);

    auto q2 = new EventQueue(m_event_queue_threshold, mapping.get_loc_count(), m_enable_stealing);
    queues.push_back(q2);

    auto* proc_arrow = new JEventProcessorArrow(ss.str()+"Tap");
    proc_arrow->set_input(q1);
    proc_arrow->set_output(q2);
    arrows.push_back(proc_arrow);
    proc_arrow->set_logger(GetLogger());
    if (found_sink) {
        proc_arrow->set_is_sink(false);
    }

    for (auto proc: procs_at_level) {
        proc_arrow->add_processor(proc);
    }

    parent_unfolder->attach_child_in(pool);
    parent_unfolder->attach_child_out(q1);
    parent_folder->attach_child_in(q2);
    parent_folder->attach_child_out(pool);
    parent_unfolder->attach(proc_arrow);
    proc_arrow->attach(parent_folder);
}


void JTopologyBuilder::attach_top_level(JEventLevel current_level) {

    std::stringstream ss;
    ss << current_level;
    auto level_str = ss.str();

    std::vector<JEventSource*> sources_at_level;
    for (JEventSource* source : m_components->get_evt_srces()) {
        if (source->GetLevel() == current_level) {
            sources_at_level.push_back(source);
        }
    }
    if (sources_at_level.size() == 0) {
        // Skip level entirely for now. Consider eventually supporting 
        // folding low levels into higher levels without corresponding unfold
        LOG_TRACE(GetLogger()) << "JTopologyBuilder: No sources found at level " << current_level << ", skipping" << LOG_END;
        JEventLevel next = next_level(current_level);
        if (next == JEventLevel::None) {
            LOG_WARN(GetLogger()) << "No sources found: Processing topology will be empty." << LOG_END;
            return;
        }
        return attach_top_level(next);
    }
    LOG_DEBUG(GetLogger()) << "JTopologyBuilder: Attaching components at top level = " << current_level << LOG_END;

    // We've now found our top level. No matter what, we need an event pool for this level
    JEventPool* pool_at_level = new JEventPool(m_components,
                                                m_event_pool_size,
                                                m_location_count,
                                                m_limit_total_events_in_flight, 
                                                current_level);
    pool_at_level->init();
    pools.push_back(pool_at_level); // Hand over ownership of the pool to the topology

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

        LOG_DEBUG(GetLogger()) << "JTopologyBuilder: No unfolders found at level " << current_level << ", finishing here." << LOG_END;

        auto queue = new EventQueue(m_event_queue_threshold, mapping.get_loc_count(), m_enable_stealing);
        queues.push_back(queue);

        auto* src_arrow = new JEventSourceArrow(level_str+"Source", sources_at_level);
        src_arrow->set_input(pool_at_level);
        src_arrow->set_output(queue);
        arrows.push_back(src_arrow);

        auto* proc_arrow = new JEventProcessorArrow(level_str+"Tap");
        proc_arrow->set_input(queue);
        proc_arrow->set_output(pool_at_level);
        arrows.push_back(proc_arrow);

        for (auto proc: procs_at_level) {
            proc_arrow->add_processor(proc);
        }
        src_arrow->attach(proc_arrow);
    }
    else if (unfolders_at_level.size() != 1) {
        throw JException("At most one unfolder must be provided for each level in the event hierarchy!");
    }
    else {
        
        auto q1 = new EventQueue(m_event_queue_threshold, mapping.get_loc_count(), m_enable_stealing);
        auto q2 = new EventQueue(m_event_queue_threshold, mapping.get_loc_count(), m_enable_stealing);

        queues.push_back(q1);
        queues.push_back(q2);

        auto *src_arrow = new JEventSourceArrow(level_str+"Source", sources_at_level);
        src_arrow->set_input(pool_at_level);
        src_arrow->set_output(q1);
        arrows.push_back(src_arrow);

        auto *map_arrow = new JEventMapArrow(level_str+"Map");
        map_arrow->set_input(q1);
        map_arrow->set_output(q2);
        arrows.push_back(map_arrow);
        src_arrow->attach(map_arrow);

        // TODO: We are using q2 temporarily knowing that it will be overwritten in attach_lower_level.
        // It would be better to rejigger how we validate PlaceRefs and accept empty placerefs/fewer ctor args
        auto *unfold_arrow = new JUnfoldArrow(level_str+"Unfold", unfolders_at_level[0], q2, pool_at_level, q2);
        arrows.push_back(unfold_arrow);
        map_arrow->attach(unfold_arrow);

        // child_in, child_out, parent_out
        auto *fold_arrow = new JFoldArrow(level_str+"Fold", current_level, unfolders_at_level[0]->GetChildLevel(), q2, pool_at_level, pool_at_level);
        // TODO: Support user-provided folders

        bool found_sink = (procs_at_level.size() > 0);
        attach_lower_level(unfolders_at_level[0]->GetChildLevel(), unfold_arrow, fold_arrow, found_sink);

        // Push fold arrow back _after_ attach_lower_level so that arrows can be iterated over in order
        arrows.push_back(fold_arrow);

        if (procs_at_level.size() != 0) {

            auto q3 = new EventQueue(m_event_queue_threshold, mapping.get_loc_count(), m_enable_stealing);
            queues.push_back(q3);

            auto* proc_arrow = new JEventProcessorArrow(level_str+"Tap");
            proc_arrow->set_input(q3);
            proc_arrow->set_output(pool_at_level);
            arrows.push_back(proc_arrow);

            for (auto proc: procs_at_level) {
                proc_arrow->add_processor(proc);
            }

            fold_arrow->attach_parent_out(q3);
            fold_arrow->attach(proc_arrow);
        }
    }

}

