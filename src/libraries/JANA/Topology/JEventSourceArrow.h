
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Topology/JPipelineArrow.h>

using Event = std::shared_ptr<JEvent>;
using EventQueue = JMailbox<Event*>;
class JEventPool;

class JEventSourceArrow : public JArrow {
private:
    std::vector<JEventSource*> m_sources;
    size_t m_current_source = 0;

    PlaceRef<Event> m_input {this, true, 1, 1};
    PlaceRef<Event> m_output {this, false, 1, 1};

public:
    JEventSourceArrow(std::string name, std::vector<JEventSource*> sources);

    void set_input(JMailbox<Event*>* queue) {
        m_input.set_queue(queue);
    }
    void set_input(JPool<Event>* pool) {
        m_input.set_pool(pool);
    }
    void set_output(JMailbox<Event*>* queue) {
        m_output.set_queue(queue);
    }
    void set_output(JPool<Event>* pool) {
        m_output.set_pool(pool);
    }

    void initialize() final;
    void finalize() final;
    void execute(JArrowMetrics& result, size_t location_id) final;
    void process(Event* event, bool& success, JArrowMetrics::Status& status);
};

