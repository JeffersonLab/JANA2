//
// Created by nbrei on 4/8/19.
//

#ifndef JANA2_JEVENTSOURCEARROW_H
#define JANA2_JEVENTSOURCEARROW_H

#include <JANA/JArrow.h>
#include <JANA/Queue.h>
#include <JANA/JEvent.h>
#include <JANA/JResourcePoolSimple.h>
#include <JANA/JFactorySet.h>

using Event = std::shared_ptr<const JEvent>;
using EventQueue = Queue<Event>;


class JEventSourceArrow : public JArrow {
private:
    JEventSource* _source;
    EventQueue* _output_queue;
    JResourcePoolSimple<JFactorySet>* _pool;
    std::vector<Event> _chunk_buffer;
    JLogger _logger;

public:
    JEventSourceArrow(std::string name, JEventSource* source, EventQueue* output_queue, JResourcePoolSimple<JFactorySet>* resourcePool);
    void initialize() final;
    void execute(JArrowMetrics& result, size_t location_id) final;
};

#endif //JANA2_JEVENTSOURCEARROW_H
