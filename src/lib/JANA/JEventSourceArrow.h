//
// Created by nbrei on 4/8/19.
//

#ifndef JANA2_JEVENTSOURCEARROW_H
#define JANA2_JEVENTSOURCEARROW_H

#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>
#include <JANA/JEvent.h>

using Arrow = greenfield::Arrow;
using Event = std::shared_ptr<const JEvent>;
using EventQueue = greenfield::Queue<Event>;

class JEventSourceArrow : public Arrow {

private:
    JEventSource& _source;
    EventQueue* _output_queue;
    std::vector<Event> _chunk_buffer;
    bool _is_initialized = false;

public:
    JEventSourceArrow(std::string name, JEventSource& source, EventQueue* output_queue);
    Arrow::Status execute();
};

#endif //JANA2_JEVENTSOURCEARROW_H
