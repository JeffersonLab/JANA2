// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Topology/JPipelineArrow.h>

class JEventPool;
class JEventSource;
class JEventUnfolder;
class JEventProcessor;
class JEvent;


class JEventMapArrow : public JPipelineArrow<JEventMapArrow> {

private:
    std::vector<JEventSource*> m_sources;
    std::vector<JEventUnfolder*> m_unfolders;
    std::vector<JEventProcessor*> m_procs;

public:
    JEventMapArrow(std::string name);

    void add_source(JEventSource* source);
    void add_unfolder(JEventUnfolder* unfolder);
    void add_processor(JEventProcessor* proc);

    void process(std::shared_ptr<JEvent>* event, bool& success, JArrowMetrics::Status& status);

    void initialize() final;
    void finalize() final;
};

