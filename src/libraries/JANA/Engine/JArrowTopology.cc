
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JArrowTopology.h"
#include "JEventProcessorArrow.h"
#include "JEventSourceArrow.h"

bool JArrowTopology::is_active() {
    for (auto arrow : arrows) {
        if (arrow->is_active()) {
            return true;
        }
    }
    return false;
}

void JArrowTopology::set_active(bool active) {
    if (active) {
        if (status == Status::Inactive) {
            status = Status::Running;
            for (auto arrow : sources) {
                // We activate our eventsources, which activate components downstream.
                arrow->initialize();
                arrow->set_active(true);
                arrow->notify_downstream(true);
            }
        }
    }
    else {
        // Someone has told us to deactivate. There are two ways to get here:
        //   * The last JEventProcessorArrow notifies us that he is deactivating because the topology is finished
        //   * The JArrowController is requesting us to stop regardless of whether the topology finished or not

        if (is_active()) {
            // Premature exit: Shut down any arrows which are still running
            status = Status::Draining;
            for (auto arrow : arrows) {
                arrow->set_active(false);
                arrow->finalize();
            }
        }
        else {
            // Arrows have all deactivated: Stop timer
            metrics.stop();
            status = Status::Inactive;
            for (auto arrow : arrows) {
                arrow->finalize();
            }
        }
    }
}

JArrowTopology::JArrowTopology() = default;

JArrowTopology::~JArrowTopology() {
    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto queue : queues) {
        delete queue;
    }
}

JArrowTopology* JArrowTopology::from_components(std::shared_ptr<JComponentManager> jcm, std::shared_ptr<JParameterManager> params, int nthreads) {

    JArrowTopology* topology = new JArrowTopology();
    topology->component_manager = jcm;  // Hold on to this
    topology->event_pool_size = nthreads;

    params->SetDefaultParameter("jana:event_pool_size", topology->event_pool_size);
    params->SetDefaultParameter("jana:limit_total_events_in_flight", topology->limit_total_events_in_flight);
    params->SetDefaultParameter("jana:event_queue_threshold", topology->event_queue_threshold);
    params->SetDefaultParameter("jana:event_source_chunksize", topology->event_source_chunksize);
    params->SetDefaultParameter("jana:event_processor_chunksize", topology->event_processor_chunksize);
    params->SetDefaultParameter("jana:enable_stealing", topology->enable_stealing);
    params->SetDefaultParameter("jana:affinity", topology->affinity);
    params->SetDefaultParameter("jana:locality", topology->locality);

    topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(topology->affinity),
                                 static_cast<JProcessorMapping::LocalityStrategy>(topology->locality));

    topology->event_pool = std::make_shared<JEventPool>(&jcm->get_fac_gens(), topology->event_pool_size,
            topology->location_count, topology->limit_total_events_in_flight);

    // Assume the simplest possible topology for now, complicate later
    auto queue = new EventQueue(topology->event_queue_threshold, topology->mapping.get_loc_count(), topology->enable_stealing);
    topology->queues.push_back(queue);

    for (auto src : jcm->get_evt_srces()) {

        JArrow* arrow = new JEventSourceArrow(src->GetName(), src, queue, topology->event_pool);
        arrow->set_backoff_tries(0);
        topology->arrows.push_back(arrow);
        topology->sources.push_back(arrow);
        arrow->set_chunksize(topology->event_source_chunksize);
        // create arrow for sources. Don't open until arrow.activate() called
    }

    auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, topology->event_pool);
    proc_arrow->set_chunksize(topology->event_processor_chunksize);
    topology->arrows.push_back(proc_arrow);

    // Receive notifications when sinks finish
    proc_arrow->attach_downstream(topology);
    topology->attach_upstream(proc_arrow);

    for (auto proc : jcm->get_evt_procs()) {
        proc_arrow->add_processor(proc);
    }
    topology->sinks.push_back(proc_arrow);


    return topology;
}
