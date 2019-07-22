//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#ifndef JANA2_PERFORMANCETESTS_H
#define JANA2_PERFORMANCETESTS_H


#include <TestTopology.h>
#include <JANA/JPerfUtils.h>
#include <SourceArrow.h>
#include <MapArrow.h>
#include <SinkArrow.h>


struct Event {
    long event_index;
    std::map<std::string, std::vector<char>> data;
    long emit_sum;
    long reduce_sum;
};

struct PerfTestSource : public Source<Event*> {
    std::string write_key;
    uint64_t latency_ms = 100;
    double latency_spread = 0;
    size_t write_bytes = 100;
    double write_spread = 0;
    long next_event_index = 0;
    long sum_over_all_events = 0;
    long message_count = 0;
    long message_count_limit = -1; // Only used when > 0

    void initialize() override {}
    void finalize() override {}

    Status inprocess(std::vector<Event*>& ts, size_t count) override {

        for (size_t i=0; i<count && (message_count_limit <= 0 || message_count < message_count_limit); ++i) {
            Event* e = new Event;
            consume_cpu_ms(latency_ms, latency_spread);
            e->emit_sum = write_memory(e->data[write_key], write_bytes, write_spread);
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
    uint64_t latency_ms = 100;
    double latency_spread = 0;
    size_t write_bytes = 100;
    double write_spread = 0;

    virtual Event* process(Event* event) {
        consume_cpu_ms(latency_ms, latency_spread);
        long sum = read_memory(event->data[read_key]);
        sum++; // Suppress compiler warning

        write_memory(event->data[write_key], write_bytes, write_spread);
        return event;
    };


};

struct PerfTestReducer : public Sink<Event*> {
    std::string read_key = "processed";
    uint64_t latency_ms = 100;
    double latency_spread = 0;
    double sum_over_all_events = 0;

    void initialize() override {}
    void finalize() override {}
    void outprocess(Event* event) override {
        consume_cpu_ms(latency_ms, latency_spread);
        event->reduce_sum = read_memory(event->data[read_key]);
        sum_over_all_events += event->reduce_sum;
        delete event; // Don't do this in the general case
    }
};

/// To be replaced with the real JParameterManager when the time is right
struct FakeParameterManager : public JService {
    using duration_t = std::chrono::steady_clock::duration;

    int chunksize;
    int backoff_tries;
    duration_t backoff_time;
    duration_t checkin_time;
};

#endif //JANA2_PERFORMANCETESTS_H
