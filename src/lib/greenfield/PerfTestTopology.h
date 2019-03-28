//
// Created by nbrei on 3/28/19.
//

#ifndef JANA2_PERFTESTTOPOLOGY_H
#define JANA2_PERFTESTTOPOLOGY_H

#include "Topology.h"
#include "Components.h"
#include "PerfUtils.h"
#include "SourceArrow.h"
#include "MapArrow.h"
#include "SinkArrow.h"

namespace greenfield {

struct Event {
    long event_index;
    std::map<std::string, std::vector<double>> data;
    double emit_sum;
    double reduce_sum;
};

class PerfTestSource : public Source<Event*> {
    std::string write_key;
    long latency;
    long latency_spread;
    long size;
    long size_spread;
    long next_event_index;
    double sum_over_all_events;

    virtual void initialize() {}
    virtual void finalize() {}

    virtual SourceStatus inprocess(std::vector<Event*>& ts, size_t count) {
        for (size_t i=0; i<count; ++i) {
            Event* e = new Event;
            consumeCPU(randint(latency-latency_spread, latency+latency_spread));
            long nitems = randint(size-size_spread, size+size_spread);
            e->emit_sum = writeMemory(e->data[write_key], nitems);
            e->event_index = next_event_index++;
            ts.push_back(e);
        }
        return SourceStatus::KeepGoing;
    }
};

class PerfTestMapper : public ParallelProcessor<Event*, Event*> {
    std::string read_key;
    std::string write_key;
    long latency;
    long latency_spread;
    long size;
    long size_spread;

    virtual Event* process(Event* event) {
        consumeCPU(randint(latency-latency_spread, latency+latency_spread));
        long sum = readMemory(event->data[read_key], event->data[read_key].size());
        long nitems = randint(size-size_spread, size+size_spread);
        writeMemory(event->data[write_key], nitems);
        return event;
    };


};

class PerfTestReducer : public Sink<Event*> {
    std::string read_key;
    long latency;
    long latency_spread;
    double sum_over_all_events;

    virtual void initialize() {}
    virtual void finalize() {}
    virtual void outprocess(Event* event) {
        consumeCPU(randint(latency-latency_spread, latency+latency_spread));
        event->reduce_sum = readMemory(event->data[read_key], event->data[read_key].size());
        sum_over_all_events += event->reduce_sum;
    }
};


class PerfTestTopology : public Topology {

public:

    PerfTestTopology() {

        auto q1 = new Queue<Event*>;
        auto q2 = new Queue<Event*>;
        auto q3 = new Queue<Event*>;

        addQueue(q1);
        addQueue(q2);
        addQueue(q3);

        q1->set_name("disentangled_event_queue");
        q2->set_name("process_queue");
        q3->set_name("histogram_queue");

        auto source = new PerfTestSource;
        auto mapper1 = new PerfTestMapper;
        auto mapper2 = new PerfTestMapper;
        auto sink = new PerfTestReducer;

        addManagedComponent(source);
        addManagedComponent(mapper1);
        addManagedComponent(mapper2);
        addManagedComponent(sink);

        addArrow(new SourceArrow<Event*>("parse", *source, q1));
        addArrow(new MapArrow<Event*, Event*>("disentangle", *mapper1, q1, q2));
        addArrow(new MapArrow<Event*, Event*>("process", *mapper2, q2, q3));
        addArrow(new SinkArrow<Event*>("plot", *sink, q3));

    }
};

}



#endif //JANA2_PERFTESTTOPOLOGY_H
