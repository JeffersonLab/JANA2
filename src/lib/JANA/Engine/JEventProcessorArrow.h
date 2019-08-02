//
// Created by nbrei on 4/8/19.
//

#ifndef JANA2_JEVENTPROCESSORARROW_H
#define JANA2_JEVENTPROCESSORARROW_H


#include <JANA/JEvent.h>
#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>

class JEventPool;

class JEventProcessorArrow : public JArrow {

public:
    using Event = std::shared_ptr<JEvent>;
    using EventQueue = JMailbox<Event>;

private:
    std::vector<JEventProcessor*> _processors;
    EventQueue* _input_queue;
    EventQueue* _output_queue;
    std::shared_ptr<JEventPool> _pool;
    JLogger _logger;

public:

    JEventProcessorArrow(std::string name,
                         EventQueue *input_queue,
                         EventQueue *output_queue,
                         std::shared_ptr<JEventPool> pool);

    void add_processor(JEventProcessor* processor);

    void initialize() final;
    void finalize() final;
    void execute(JArrowMetrics& result, size_t location_id) final;

    size_t get_pending() final;
    size_t get_threshold() final;
    void set_threshold(size_t) final;

};


#endif //JANA2_JEVENTPROCESSORARROW_H
