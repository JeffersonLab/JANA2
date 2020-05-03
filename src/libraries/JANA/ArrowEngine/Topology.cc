
#include <JANA/ArrowEngine/Topology.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/ArrowEngine/ComponentArrows.h>
#include <JANA/JEventProcessor.h>

namespace jana {
namespace arrowengine {


void DefaultJanaTopology::initialize() {

    // Construct arrow topology from JCM.
    // Get granularity from JPM
    // Get arrows from JCM
    // Maintain a queue of event sources somewhere here
    // Need to figure out event pool again as well :(

    size_t event_queue_threshold = 80;
    size_t event_source_chunksize = 40;
    size_t event_processor_chunksize = 1;
    size_t location_count = 1;
    bool enable_stealing = false;
    bool limit_total_events_in_flight = true;
    int affinity = 0;
    int locality = 0;

    event_pool_size = params->GetParameterValue<size_t>("nthreads");
    params->SetDefaultParameter("jana:event_pool_size", event_pool_size);
    params->SetDefaultParameter("jana:limit_total_events_in_flight", limit_total_events_in_flight);
    params->SetDefaultParameter("jana:event_queue_threshold", event_queue_threshold);
    params->SetDefaultParameter("jana:event_source_chunksize", event_source_chunksize);
    params->SetDefaultParameter("jana:event_processor_chunksize", event_processor_chunksize);
    params->SetDefaultParameter("jana:enable_stealing", enable_stealing);
    params->SetDefaultParameter("jana:affinity", affinity);
    params->SetDefaultParameter("jana:locality", locality);

    mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(affinity),
                       static_cast<JProcessorMapping::LocalityStrategy>(locality));

    event_pool = std::make_shared<JEventPool>(&components->get_fac_gens(), event_pool_size, location_count, limit_total_events_in_flight);

    // Assume the simplest possible topology for now, complicate later
    auto queue = new EventQueue(event_queue_threshold, mapping.get_loc_count(), enable_stealing);

    for (auto src : components->get_evt_srces()) {

        Arrow* arrow = new SourceArrow<Event, EventSourceOp>(src->GetName(), false, src, event_pool);

        //arrow->set_backoff_tries(0);
        arrows.push_back(arrow);
        arrow->chunk_size = event_source_chunksize;
    }

    Arrow* proc_arrow = new StageArrow<Event, Event, EventProcessorOp>("processors", true, components->get_evt_procs(), event_pool);
    proc_arrow->chunk_size = event_processor_chunksize;
    arrows.push_back(proc_arrow);
}

void DefaultJanaTopology::open() {

    // Open event processors, but do NOT open event sources yet
    for (auto proc : components->get_evt_procs()) {
        proc->DoInitialize();
    }
}

void DefaultJanaTopology::close() {
    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto proc : components->get_evt_procs()) {
        proc->DoFinalize();
    }
}

} // namespace arrowengine
} // namespace jana

