// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Topology/JTriggeredArrow.h"

class JEventProcessor;
class JEvent;


class JEventTapArrow : public JTriggeredArrow<JEventTapArrow> {

private:
    std::vector<JEventProcessor*> m_procs;
    Place m_input {this, true };
    Place m_output {this, false };

public:
    JEventTapArrow(std::string name);

    void add_processor(JEventProcessor* proc);

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status);
    void initialize() final;
    void finalize() final;
};

