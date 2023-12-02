
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JEVENTPROCESSORARROW_H
#define JANA2_JEVENTPROCESSORARROW_H


#include <JANA/JEventProcessor.h>
#include <JANA/Engine/JPipelineArrow.h>

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
                         std::shared_ptr<JEventPool> pool);

    void add_processor(JEventProcessor* processor);

    JArrowMetrics::Status process(Event* event);

    void initialize() final;
    void finalize() final;
};


#endif //JANA2_JEVENTPROCESSORARROW_H
