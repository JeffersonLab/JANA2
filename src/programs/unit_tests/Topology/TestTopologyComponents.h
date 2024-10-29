
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Topology/JArrowMetrics.h"
#include "JANA/Topology/JTriggeredArrow.h"
#include <JANA/JEvent.h>

struct EventData {
    int x = 0;
    double y = 0.0;
    double z = 0.0;
};

struct RandIntArrow : public JTriggeredArrow<RandIntArrow> {

    size_t emit_limit = 20;  // How many to emit
    size_t emit_count = 0;   // How many emitted so far
    int emit_sum = 0;        // Sum of all ints emitted so far

    RandIntArrow(std::string name, JEventPool* pool, JMailbox<JEvent*>* output_queue) {
        set_name(name);
        set_is_source(true);
        create_ports(1, 1);
        attach(pool, 0);
        attach(output_queue, 1);
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) {

        if (emit_count >= emit_limit) {
            outputs[0] = {event, 0}; // Send event back to the input pool
            output_count = 1;
            status = JArrowMetrics::Status::Finished;
            return;
        }

        auto data = new EventData {7};
        event->Insert(data, "first");

        emit_sum += data->x;
        emit_count += 1;

        outputs[0] = {event, 1}; // Send event back to the input pool
        output_count = 1;
        status = (emit_count == emit_limit) ? JArrowMetrics::Status::Finished : JArrowMetrics::Status::KeepGoing;
        // This design lets us declare Finished immediately on the last event, instead of after

        LOG_DEBUG(JArrow::m_logger) << "RandIntSource emitted event " << emit_count << " with value " << data->x << LOG_END;
    }
};


struct MultByTwoArrow : public JTriggeredArrow<MultByTwoArrow> {

    MultByTwoArrow(std::string name, JMailbox<JEvent*>* input_queue, JMailbox<JEvent*>* output_queue) {
        set_name(name);
        set_is_parallel(true);
        create_ports(1, 1);
        attach(input_queue, 0);
        attach(output_queue, 1);
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) {
        auto prev = event->Get<EventData>("first");
        auto x = prev.at(0)->x;
        auto next = new EventData { .x=x, .y=x*2.0 };
        event->Insert(next, "second");

        outputs[0] = {event, 1};
        output_count = 1;
        status = JArrowMetrics::Status::KeepGoing;
    }
};

struct SubOneArrow : public JTriggeredArrow<SubOneArrow> {
    
    SubOneArrow(std::string name, JMailbox<JEvent*>* input_queue, JMailbox<JEvent*>* output_queue) {
        set_name(name);
        set_is_parallel(true);
        create_ports(1, 1);
        attach(input_queue, 0);
        attach(output_queue, 1);
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) {
        auto prev = event->Get<EventData>("second");
        auto x = prev.at(0)->x;
        auto y = prev.at(0)->y;
        auto z = y - 1;
        auto next = new EventData { .x=x, .y=y, .z=z };
        event->Insert(next, "third");

        outputs[0] = {event, 1};
        output_count = 1;
        status = JArrowMetrics::Status::KeepGoing;
    }
};

struct SumArrow : public JTriggeredArrow<SumArrow> {

    double sum = 0;

    SumArrow(std::string name, JMailbox<JEvent*>* input_queue, JEventPool* pool) {
        set_name(name);
        set_is_sink(true);
        create_ports(1, 1);
        attach(input_queue, 0);
        attach(pool, 1);
    }

    void fire(JEvent* event, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& status) {
        auto prev = event->Get<EventData>("third");
        auto z = prev.at(0)->z;
        sum += z;

        outputs[0] = {event, 1};
        output_count = 1;
        status = JArrowMetrics::Status::KeepGoing;
    }
};


