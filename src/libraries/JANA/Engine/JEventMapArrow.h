// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Engine/JPipelineArrow.h>

class JEventPool;
class JEventSource;
class JEventUnfolder;
class JEvent;

using Event = std::shared_ptr<JEvent>;
using EventQueue = JMailbox<Event*>;

class JEventMapArrow : public JPipelineArrow<JEventMapArrow, Event> {

private:
    std::vector<JEventSource*> m_sources;
    std::vector<JEventUnfolder*> m_unfolders;

public:
    JEventMapArrow(std::string name, EventQueue *input_queue, EventQueue *output_queue);

    void add_source(JEventSource* source);
    void add_unfolder(JEventUnfolder* unfolder);

    void process(Event* event, bool& success, JArrowMetrics::Status& status);

    void initialize() final;
    void finalize() final;
};

