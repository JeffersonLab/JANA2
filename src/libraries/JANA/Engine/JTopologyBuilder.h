
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
        JLogger m_logger;
	JArrowTopology* m_override = nullptr; // Non-owning; caller responsible for deletion.

public:

	JTopologyBuilder() = default;
	~JTopologyBuilder() override = default;

	inline JArrowTopology* get_or_create(int nthreads) {
		if (m_override != nullptr) {
			return m_override;
		}
		return build(nthreads);
	}

	inline void set_override(JArrowTopology* topology) {
		m_override = topology;
	}

	void acquire_services(JServiceLocator* sl) override {
		m_components = sl->get<JComponentManager>();
		m_params = sl->get<JParameterManager>();
                m_logger = sl->get<JLoggingService>()->get_logger("JArrow");
	};

	inline virtual JArrowTopology* build(int nthreads) {

		auto topology = new JArrowTopology;
		topology->component_manager = m_components;  // Ensure the lifespan of the component manager exceeds that of the topology

		size_t event_pool_size = nthreads;
		size_t event_queue_threshold = 80;
		size_t event_source_chunksize = 40;
		size_t event_processor_chunksize = 1;
		size_t location_count = 1;
                bool enable_call_graph_recording = false;
                bool enable_stealing = false;
		bool limit_total_events_in_flight = true;
		int affinity = 2;
		int locality = 0;

		m_params->SetDefaultParameter("jana:event_pool_size", event_pool_size);
		m_params->SetDefaultParameter("jana:limit_total_events_in_flight", limit_total_events_in_flight);
		m_params->SetDefaultParameter("jana:event_queue_threshold", event_queue_threshold);
		m_params->SetDefaultParameter("jana:event_source_chunksize", event_source_chunksize);
		m_params->SetDefaultParameter("jana:event_processor_chunksize", event_processor_chunksize);
		m_params->SetDefaultParameter("jana:enable_stealing", enable_stealing);
		m_params->SetDefaultParameter("jana:affinity", affinity);
		m_params->SetDefaultParameter("jana:locality", locality);
		m_params->SetDefaultParameter("RECORD_CALL_STACK", enable_call_graph_recording);


            // TODO: Move params onto JProcessorTopology. Maybe do the same for nthreads actually
		topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(affinity),
		                             static_cast<JProcessorMapping::LocalityStrategy>(locality));

		topology->event_pool = std::make_shared<JEventPool>(&m_components->get_fac_gens(),
                                                                    enable_call_graph_recording,
                                                                    event_pool_size,
		                                                    location_count,
                                                                    limit_total_events_in_flight);

		// Assume the simplest possible topology for now, complicate later
		auto queue = new EventQueue(event_queue_threshold, topology->mapping.get_loc_count(), enable_stealing);
		topology->queues.push_back(queue);

		for (auto src : m_components->get_evt_srces()) {

			// create arrow for each source. Don't open until arrow.run() called
			JArrow *arrow = new JEventSourceArrow(src->GetName(), src, queue, topology->event_pool);
			arrow->set_backoff_tries(0);
                        arrow->set_running_arrows(&topology->running_arrow_count);
			topology->arrows.push_back(arrow);
			topology->sources.push_back(arrow);
			arrow->set_chunksize(event_source_chunksize);
                        arrow->set_logger(m_logger);
		}

		auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, topology->event_pool);
                for (auto src_arrow : topology->sources) {
                    src_arrow->attach(proc_arrow);
                }
		proc_arrow->set_chunksize(event_processor_chunksize);
		topology->arrows.push_back(proc_arrow);
                proc_arrow->set_running_arrows(&topology->running_arrow_count);
                proc_arrow->set_logger(m_logger);

                for (auto proc : m_components->get_evt_procs()) {
			proc_arrow->add_processor(proc);
		}
		topology->sinks.push_back(proc_arrow);

		return topology;
	}

};


#endif //JANA2_JTOPOLOGYBUILDER_H
