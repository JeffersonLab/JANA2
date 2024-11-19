// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JArrow.h>

class JEventPool;
class JEventSource;
class JEventUnfolder;
class JEventProcessor;
class JEvent;


class JEventMapArrow : public JArrow {

public:
    enum PortIndex {EVENT_IN=0, EVENT_OUT=1};

private:
    std::vector<JEventSource*> m_sources;
    std::vector<JEventUnfolder*> m_unfolders;
    std::vector<JEventProcessor*> m_procs;

public:
    JEventMapArrow(std::string name);

    void add_source(JEventSource* source);
    void add_unfolder(JEventUnfolder* unfolder);
    void add_processor(JEventProcessor* proc);

    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status);

    void initialize() final;
    void finalize() final;
};

