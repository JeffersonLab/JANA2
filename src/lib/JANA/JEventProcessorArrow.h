//
// Created by nbrei on 4/8/19.
//

#ifndef JANA2_JEVENTPROCESSORARROW_H
#define JANA2_JEVENTPROCESSORARROW_H


#include <JANA/JEvent.h>
#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>

using Arrow = greenfield::Arrow;
using Event = std::shared_ptr<JEvent>;
using EventQueue = greenfield::Queue<Event>;

class JEventProcessorArrow : public Arrow {

private:
    JEventProcessor& _processor;
    EventQueue* _input_queue;
    EventQueue* _output_queue;

public:

    JEventProcessorArrow(std::string name,
                         JEventProcessor& processor,
                         EventQueue *input_queue,
                         EventQueue *output_queue);

    Arrow::Status execute();

};


#endif //JANA2_JEVENTPROCESSORARROW_H
