
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JTopologyBuilder.h"
#include "JEventSourceArrow.h"
#include "JEventProcessorArrow.h"

JArrowTopology *
JTopologyBuilder::build(int nthreads) {

	auto topology = new JArrowTopology;
	topology->component_manager = m_components;  // Ensure the lifespan of the component manager exceeds that of the topology

	size_t event_pool_size = nthreads;
	size_t event_queue_threshold = 80;
	size_t event_source_chunksize = 40;
	size_t event_processor_chunksize = 1;
	size_t location_count = 1;
	bool enable_stealing = false;
	bool limit_total_events_in_flight = true;
	int affinity = 0;
	int locality = 0;

	m_params->SetDefaultParameter("jana:event_pool_size", event_pool_size);
	m_params->SetDefaultParameter("jana:limit_total_events_in_flight", limit_total_events_in_flight);
	m_params->SetDefaultParameter("jana:event_queue_threshold", event_queue_threshold);
	m_params->SetDefaultParameter("jana:event_source_chunksize", event_source_chunksize);
	m_params->SetDefaultParameter("jana:event_processor_chunksize", event_processor_chunksize);
	m_params->SetDefaultParameter("jana:enable_stealing", enable_stealing);
	m_params->SetDefaultParameter("jana:affinity", affinity);
	m_params->SetDefaultParameter("jana:locality", locality);

	// TODO: Move params onto JProcessorTopology. Maybe do the same for nthreads actually
	topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(affinity),
	                             static_cast<JProcessorMapping::LocalityStrategy>(locality));

	topology->event_pool = std::make_shared<JEventPool>(&m_components->get_fac_gens(), event_pool_size,
	                                                    location_count, limit_total_events_in_flight);

	// Assume the simplest possible topology for now, complicate later
	auto queue = new EventQueue(event_queue_threshold, topology->mapping.get_loc_count(), enable_stealing);
	topology->queues.push_back(queue);

	for (auto src : m_components->get_evt_srces()) {

		// create arrow for each source. Don't open until arrow.activate() called
		JArrow *arrow = new JEventSourceArrow(src->GetName(), src, queue, topology->event_pool);
		arrow->set_backoff_tries(0);
		topology->arrows.push_back(arrow);
		topology->sources.push_back(arrow);
		arrow->set_chunksize(event_source_chunksize);
	}

	auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, topology->event_pool);
	proc_arrow->set_chunksize(event_processor_chunksize);
	topology->arrows.push_back(proc_arrow);

	// Receive notifications when sinks finish
	proc_arrow->attach_downstream(topology);   // TODO: Simplify shutdown process using upstream count instead
	topology->attach_upstream(proc_arrow);

	for (auto proc : m_components->get_evt_procs()) {
		proc_arrow->add_processor(proc);
	}
	topology->sinks.push_back(proc_arrow);

	return topology;
}
