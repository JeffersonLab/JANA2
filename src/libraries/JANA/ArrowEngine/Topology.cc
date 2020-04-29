#include <JANA/ArrowEngine/Topology.h>

namespace jana {
namespace arrowengine {


const std::vector<JArrow *> & DefaultJanaTopology::get_arrows() {

}

void DefaultJanaTopology::initialize() {

    // Construct arrow topology from JCM.
    // Get granularity from JPM
    // Get arrows from somewhere
    // Maintain a queue of event sources somewhere here
    // Need to figure out event pool again as well :(

    size_t event_pool_size = nthreads;
    size_t event_queue_threshold = 80;
    size_t event_source_chunksize = 40;
    size_t event_processor_chunksize = 1;
    size_t location_count = 1;
    bool enable_stealing = false;
    bool limit_total_events_in_flight = true;
    int affinity = 0;
    int locality = 0;

    params->SetDefaultParameter("jana:event_pool_size", event_pool_size);
    params->SetDefaultParameter("jana:limit_total_events_in_flight", limit_total_events_in_flight);
    params->SetDefaultParameter("jana:event_queue_threshold", event_queue_threshold);
    params->SetDefaultParameter("jana:event_source_chunksize", event_source_chunksize);
    params->SetDefaultParameter("jana:event_processor_chunksize", event_processor_chunksize);
    params->SetDefaultParameter("jana:enable_stealing", enable_stealing);
    params->SetDefaultParameter("jana:affinity", affinity);
    params->SetDefaultParameter("jana:locality", locality);

    topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(affinity),
                                 static_cast<JProcessorMapping::LocalityStrategy>(locality));

    topology->event_pool = std::make_shared<JEventPool>(&jcm->get_fac_gens(), event_pool_size,
                                                        location_count, limit_total_events_in_flight);

    // Assume the simplest possible topology for now, complicate later
    auto queue = new EventQueue(event_queue_threshold, topology->mapping.get_loc_count(), enable_stealing);
    topology->queues.push_back(queue);

    for (auto src : jcm->get_evt_srces()) {

        JArrow* arrow = new JEventSourceArrow(src->GetName(), src, queue, topology->event_pool);
        arrow->set_backoff_tries(0);
        topology->arrows.push_back(arrow);
        topology->sources.push_back(arrow);
        arrow->set_chunksize(event_source_chunksize);
        // create arrow for sources. Don't open until arrow.activate() called
    }

    auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, topology->event_pool);
    proc_arrow->set_chunksize(event_processor_chunksize);
    topology->arrows.push_back(proc_arrow);

    // Receive notifications when sinks finish
    proc_arrow->attach_downstream(topology);
    topology->attach_upstream(proc_arrow);

    for (auto proc : jcm->get_evt_procs()) {
        proc_arrow->add_processor(proc);
    }
    topology->sinks.push_back(proc_arrow);


}

void DefaultJanaTopology::open() {

    // open event processors
    // DON'T open event sources
    // start timer
}

void DefaultJanaTopology::close() {
    for (auto arrow : arrows) {
        delete arrow;
    }
    // close event processors
    // Scheduler calls me? Who calls me?
}

} // namespace arrowengine
} // namespace jana

