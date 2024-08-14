// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JPipelineArrow.h>

class JEventPool;
class JEventProcessor;
class JEvent;

using Event = std::shared_ptr<JEvent>;
using EventQueue = JMailbox<Event*>;

class JEventTapArrow : public JPipelineArrow<JEventTapArrow, Event> {

private:
    std::vector<JEventProcessor*> m_procs;

public:
    JEventTapArrow(std::string name, EventQueue *input_queue, EventQueue *output_queue, JEventPool *pool);

    void add_processor(JEventProcessor* proc);
    void process(Event* event, bool& success, JArrowMetrics::Status& status);
    void initialize() final;
    void finalize() final;
};

