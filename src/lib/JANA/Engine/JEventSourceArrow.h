//
// Created by nbrei on 4/8/19.
//

#ifndef JANA2_JEVENTSOURCEARROW_H
#define JANA2_JEVENTSOURCEARROW_H

#include <JANA/JFactorySet.h>

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>

using Event = std::shared_ptr<JEvent>;
using EventQueue = JMailbox<Event>;

class JEventPool;

class JEventSourceArrow : public JArrow {
private:
    JAbstractEventSource* _source;
    EventQueue* _output_queue;
    std::shared_ptr<JEventPool> _pool;
    std::vector<Event> _chunk_buffer;
    JLogger _logger;

public:
    JEventSourceArrow(std::string name, JAbstractEventSource* source, EventQueue* output_queue, std::shared_ptr<JEventPool> pool);
    void initialize() final;
    void execute(JArrowMetrics& result, size_t location_id) final;
};

#endif //JANA2_JEVENTSOURCEARROW_H
