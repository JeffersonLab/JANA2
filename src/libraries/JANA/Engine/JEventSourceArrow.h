
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTSOURCEARROW_H
#define JANA2_JEVENTSOURCEARROW_H

#include <JANA/JFactorySet.h>

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>

using Event = std::shared_ptr<JEvent>*;
using EventQueue = JMailbox<Event>;

class JEventPool;

class JEventSourceArrow : public JArrow {
private:
    std::vector<JEventSource*> m_sources;
    size_t m_current_source = 0;
    EventQueue* m_output_queue;
    std::shared_ptr<JEventPool> m_pool;
    std::vector<Event> m_chunk_buffer;

public:
    JEventSourceArrow(std::string name, std::vector<JEventSource*> sources, EventQueue* output_queue, std::shared_ptr<JEventPool> pool);
    void initialize() final;
    void finalize() final;
    void execute(JArrowMetrics& result, size_t location_id) final;
};

#endif //JANA2_JEVENTSOURCEARROW_H
