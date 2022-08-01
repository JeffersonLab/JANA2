
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/Engine/JTopologyBuilder.h>
#include <JANA/Engine/JBlockSourceArrow.h>
#include <JANA/Engine/JBlockDisentanglerArrow.h>
#include <JANA/Engine/JEventProcessorArrow.h>

#include "BlockExampleSource.h"
#include "BlockExampleProcessor.h"


int main() {

	JApplication app;

	auto source = new BlockExampleSource;
	auto processor = new BlockExampleProcessor;
	auto topology = new JArrowTopology;

	auto block_queue = new JMailbox<MyBlock*>;
	auto event_queue = new JMailbox<std::shared_ptr<JEvent>>;

	topology->component_manager = app.GetService<JComponentManager>();  // Ensure the lifespan of the component manager exceeds that of the topology
	topology->event_pool = std::make_shared<JEventPool>(topology->component_manager, 20, 1, true);

	auto block_source_arrow = new JBlockSourceArrow<MyBlock>("block_source", source, block_queue);
	auto block_disentangler_arrow = new JBlockDisentanglerArrow<MyBlock>("block_disentangler", source, block_queue, event_queue, topology->event_pool);
	auto processor_arrow = new JEventProcessorArrow("processors", event_queue, nullptr, topology->event_pool);

	processor_arrow->add_processor(processor);

	topology->arrows.push_back(block_source_arrow);
	topology->arrows.push_back(block_disentangler_arrow);
	topology->arrows.push_back(processor_arrow);

	topology->sources.push_back(block_source_arrow);
	topology->sinks.push_back(processor_arrow);

	auto builder = app.GetService<JTopologyBuilder>();
	builder->set_override(topology);

	app.Run(true);
}

