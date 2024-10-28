// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JTriggeredArrow.h>

class JEventPool;
class JEventSource;
class JEventUnfolder;
class JEventProcessor;
class JEvent;


class JEventMapArrow : public JTriggeredArrow<JEventMapArrow> {

private:
    std::vector<JEventSource*> m_sources;
    std::vector<JEventUnfolder*> m_unfolders;
    std::vector<JEventProcessor*> m_procs;

public:
    JEventMapArrow(std::string name);

    void add_source(JEventSource* source);
    void add_unfolder(JEventUnfolder* unfolder);
    void add_processor(JEventProcessor* proc);

    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status);

    void initialize() final;
    void finalize() final;
};

