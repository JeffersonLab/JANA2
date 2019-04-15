//
// Created by nbrei on 4/8/19.
//

#ifndef JANA2_JEVENTSOURCEARROW_H
#define JANA2_JEVENTSOURCEARROW_H

#include <JANA/JArrow.h>
#include <JANA/Queue.h>
#include <JANA/JEvent.h>

using Event = std::shared_ptr<const JEvent>;
using EventQueue = Queue<Event>;

class JEventSourceArrow : public JArrow {

private:
    JEventSource* _source;
    EventQueue* _output_queue;
    JApplication* _app;
    std::vector<Event> _chunk_buffer;
    bool _is_initialized = false;

public:
    JEventSourceArrow(std::string name, JEventSource* source, EventQueue* output_queue, JApplication* app);
    JArrow::Status execute();
};

#endif //JANA2_JEVENTSOURCEARROW_H
