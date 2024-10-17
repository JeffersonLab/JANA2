
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JEventProcessor.h>
#include <JANA/Topology/JPipelineArrow.h>


class JEventProcessorArrow : public JPipelineArrow<JEventProcessorArrow> {

private:
    std::vector<JEventProcessor*> m_processors;

public:
    JEventProcessorArrow(std::string name);
    void add_processor(JEventProcessor* processor);
    void process(std::shared_ptr<JEvent>* event, bool& success, JArrowMetrics::Status& status);
    void initialize() final;
    void finalize() final;
};

