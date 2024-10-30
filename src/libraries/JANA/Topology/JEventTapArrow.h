// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrow.h>

class JEventProcessor;
class JEvent;


class JEventTapArrow : public JArrow {
public:
    enum PortIndex {EVENT_IN=0, EVENT_OUT=1};

private:
    std::vector<JEventProcessor*> m_procs;

public:
    JEventTapArrow(std::string name);

    void add_processor(JEventProcessor* proc);

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) final;
    void initialize() final;
    void finalize() final;
};

