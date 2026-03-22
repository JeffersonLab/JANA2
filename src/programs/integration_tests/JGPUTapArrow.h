// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrow.h>
#include "JContinuation.h"

class JEventProcessor;
class JEvent;


class JGPUTapArrow : public JArrow {
public:
    enum PortIndex {EVENT_IN=0, EVENT_OUT=1};

private:

    std::map<std::string, std::string> m_fake_procs;
    //std::vector<JEventProcessor*> m_procs;

public:
    JGPUTapArrow(std::string name);

    //void add_processor(JEventProcessor* proc);

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) final;
    void initialize() final;
    void finalize() final;
};

