//
// Created by nbrei on 4/8/19.
//

#ifndef JANA2_JEVENTPROCESSORARROW_H
#define JANA2_JEVENTPROCESSORARROW_H


#include <JANA/JEvent.h>
#include <JANA/JArrow.h>
#include <JANA/Queue.h>

using Event = std::shared_ptr<const JEvent>;
using EventQueue = Queue<Event>;

class JEventProcessorArrow : public JArrow {

private:
    std::vector<JEventProcessor*> _processors;
    EventQueue* _input_queue;
    EventQueue* _output_queue;

public:

    JEventProcessorArrow(std::string name,
                         EventQueue *input_queue,
                         EventQueue *output_queue);

    void add_processor(JEventProcessor* processor);

    JArrow::Status execute();

};


#endif //JANA2_JEVENTPROCESSORARROW_H
