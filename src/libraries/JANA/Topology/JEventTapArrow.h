// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JPipelineArrow.h>

class JEventProcessor;
class JEvent;


class JEventTapArrow : public JPipelineArrow<JEventTapArrow> {

private:
    std::vector<JEventProcessor*> m_procs;

public:
    JEventTapArrow(std::string name);

    void add_processor(JEventProcessor* proc);
    void process(JEvent* event, bool& success, JArrowMetrics::Status& status);
    void initialize() final;
    void finalize() final;
};

