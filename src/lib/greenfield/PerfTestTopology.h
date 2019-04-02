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
    std::map<std::string, std::vector<char>> data;
    long emit_sum;
    long reduce_sum;
};

struct PerfTestSource : public Source<Event*> {
    std::string write_key;
    int latency_ms = 100;
    int latency_spread = 0;
    int write_bytes = 100;
    int write_spread = 0;
    long next_event_index = 0;
    long sum_over_all_events = 0;

    virtual void initialize() {}
    virtual void finalize() {}

    virtual SourceStatus inprocess(std::vector<Event*>& ts, size_t count) {
        for (size_t i=0; i<count; ++i) {
            Event* e = new Event;
            consume_cpu_ms(randint(latency_ms-latency_spread, latency_ms+latency_spread));
            int bytes = randint(write_bytes-write_spread, write_bytes+write_spread);
            e->emit_sum = writeMemory(e->data[write_key], bytes);
            e->event_index = next_event_index++;
            ts.push_back(e);
        }
        return SourceStatus::KeepGoing;
    }
};

struct PerfTestMapper : public ParallelProcessor<Event*, Event*> {
    std::string read_key = "disentangled";
    std::string write_key = "processed";
    long latency_ms = 100;
    long latency_spread = 0;
    long write_bytes = 100;
    long write_spread = 0;

    virtual Event* process(Event* event) {
        consume_cpu_ms(randint(latency_ms-latency_spread, latency_ms+latency_spread));
        long sum = readMemory(event->data[read_key], event->data[read_key].size());
        sum++; // Suppress compiler warning
        long bytes = randint(write_bytes-write_spread, write_bytes+write_spread);
        writeMemory(event->data[write_key], bytes);
        return event;
    };


};

struct PerfTestReducer : public Sink<Event*> {
    std::string read_key = "processed";
    long latency_ms = 100;
    long latency_spread = 0;
    double sum_over_all_events = 0;

    virtual void initialize() {}
    virtual void finalize() {}
    virtual void outprocess(Event* event) {
        consume_cpu_ms(randint(latency_ms-latency_spread, latency_ms+latency_spread));
        event->reduce_sum = readMemory(event->data[read_key], event->data[read_key].size());
        sum_over_all_events += event->reduce_sum;
        delete event; // Don't do this in the general case
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
        addArrow(new SinkArrow<Event*>("plot", *sink, q3), true);

    }
};

}



#endif //JANA2_PERFTESTTOPOLOGY_H
