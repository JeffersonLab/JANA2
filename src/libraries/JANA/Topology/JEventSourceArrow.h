
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Topology/JArrow.h>


class JEventSourceArrow : public JArrow {
public:
    enum PortIndex {EVENT_IN=0, EVENT_OUT=1};

private:
    std::vector<JEventSource*> m_sources;
    size_t m_current_source = 0;
    bool m_barrier_active = false;
    JEvent* m_pending_barrier_event = nullptr;

public:
    JEventSourceArrow(std::string name, std::vector<JEventSource*> sources);

    void initialize() final;
    void finalize() final;
    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status);
};

