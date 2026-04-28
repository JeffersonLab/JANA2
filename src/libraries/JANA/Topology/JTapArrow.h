// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Utils/JEventLevel.h"
#include <JANA/Topology/JArrow.h>

class JEventProcessor;
class JEvent;


class JTapArrow : public JArrow {
public:
    enum PortIndex {EVENT_IN=0, EVENT_OUT=1};

private:
    std::vector<JEventProcessor*> m_procs;

public:
    JTapArrow(std::string name, JEventLevel level=JEventLevel::PhysicsEvent);

    void AddProcessor(JEventProcessor* proc);

    void Fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) final;
    void Initialize() final;
    void Finalize() final;
};

