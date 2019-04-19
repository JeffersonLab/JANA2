//
// Created by nbrei on 3/28/19.
//

#ifndef JANA2_PERFTESTTOPOLOGY_H
#define JANA2_PERFTESTTOPOLOGY_H

#include <JANA/JTopology.h>
#include <JANA/JPerfUtils.h>
#include <greenfield/Components.h>
#include <greenfield/SourceArrow.h>
#include <greenfield/MapArrow.h>
#include <greenfield/SinkArrow.h>


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
    long message_count = 0;
    long message_count_limit = -1; // Only used when > 0

    virtual void initialize() {}
    virtual void finalize() {}

    virtual Status inprocess(std::vector<Event*>& ts, size_t count) {

        for (size_t i=0; i<count && (message_count_limit <= 0 || message_count < message_count_limit); ++i) {
            Event* e = new Event;
            consume_cpu_ms(randint(latency_ms-latency_spread, latency_ms+latency_spread));
            int bytes = randint(write_bytes-write_spread, write_bytes+write_spread);
            e->emit_sum = writeMemory(e->data[write_key], bytes);
            sum_over_all_events += e->emit_sum;
            e->event_index = next_event_index++;
            ts.push_back(e);
            message_count++;
        }
        if (message_count_limit > 0 && message_count >= message_count_limit) {
            return Status::Finished;
        }
        return Status::KeepGoing;
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


#endif //JANA2_PERFTESTTOPOLOGY_H
