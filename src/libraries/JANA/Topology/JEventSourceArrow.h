
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Topology/JPipelineArrow.h>

using Event = std::shared_ptr<JEvent>;
using EventQueue = JMailbox<Event*>;
class JEventPool;

class JEventSourceArrow : public JPipelineArrow<JEventSourceArrow, Event> {
private:
    std::vector<JEventSource*> m_sources;
    size_t m_current_source = 0;

public:
    JEventSourceArrow(std::string name, std::vector<JEventSource*> sources, EventQueue* output_queue, JEventPool* pool);
    void initialize() final;
    void finalize() final;

    void process(Event* event, bool& success, JArrowMetrics::Status& status);
};

