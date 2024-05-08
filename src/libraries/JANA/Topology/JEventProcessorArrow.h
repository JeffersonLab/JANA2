
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JEventProcessor.h>
#include <JANA/Topology/JPipelineArrow.h>

class JEventPool;

using Event = std::shared_ptr<JEvent>;
using EventQueue = JMailbox<Event*>;

class JEventProcessorArrow : public JPipelineArrow<JEventProcessorArrow, Event> {

private:
    std::vector<JEventProcessor*> m_processors;

public:
    JEventProcessorArrow(std::string name,
                         EventQueue *input_queue,
                         EventQueue *output_queue,
                         JEventPool *pool);

    void add_processor(JEventProcessor* processor);

    void process(Event* event, bool& success, JArrowMetrics::Status& status);

    void initialize() final;
    void finalize() final;
};

