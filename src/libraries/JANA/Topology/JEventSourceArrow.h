
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Topology/JTriggeredArrow.h>


class JEventSourceArrow : public JTriggeredArrow<JEventSourceArrow> {
public:
    const size_t EVENT_IN = 0;
    const size_t EVENT_OUT = 1;

private:
    std::vector<JEventSource*> m_sources;
    size_t m_current_source = 0;
    bool m_barrier_active = false;
    JEvent* m_pending_barrier_event = nullptr;

public:
    JEventSourceArrow(std::string name, std::vector<JEventSource*> sources);

    void set_input(JMailbox<JEvent*>* queue) {
        m_ports[EVENT_IN].queue = queue;
        m_ports[EVENT_IN].pool = nullptr;
    }
    void set_input(JEventPool* pool) {
        m_ports[EVENT_IN].queue = nullptr;
        m_ports[EVENT_IN].pool = pool;
    }
    void set_output(JMailbox<JEvent*>* queue) {
        m_ports[EVENT_OUT].queue = queue;
        m_ports[EVENT_OUT].pool = nullptr;
    }
    void set_output(JEventPool* pool) {
        m_ports[EVENT_OUT].queue = nullptr;
        m_ports[EVENT_OUT].pool = pool;
    }

    void initialize() final;
    void finalize() final;
    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status);
};

