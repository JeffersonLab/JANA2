
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include "JANA/Topology/JArrowMetrics.h"
#include <JANA/Topology/JPipelineArrow.h>
#include <JANA/JEvent.h>

struct EventData {
    int x = 0;
    double y = 0.0;
    double z = 0.0;
};

struct RandIntArrow : public JPipelineArrow<RandIntArrow> {

    size_t emit_limit = 20;  // How many to emit
    size_t emit_count = 0;   // How many emitted so far
    int emit_sum = 0;        // Sum of all ints emitted so far

    RandIntArrow(std::string name, JEventPool* pool, JMailbox<JEvent*>* output_queue)
        : JPipelineArrow(name, false, true, false) {
        this->set_input(pool);
        this->set_output(output_queue);
    }

    void process(JEvent* event, bool& success, JArrowMetrics::Status& status) {

        if (emit_count >= emit_limit) {
            success = false;
            status = JArrowMetrics::Status::Finished;
            return;
        }

        auto data = new EventData {7};

        event->Insert(data, "first");

        emit_sum += data->x;
        emit_count += 1;
        LOG_DEBUG(JArrow::m_logger) << "RandIntSource emitted event " << emit_count << " with value " << data->x << LOG_END;
        success = true;
        status = (emit_count == emit_limit) ? JArrowMetrics::Status::Finished : JArrowMetrics::Status::KeepGoing;
        // This design lets us declare Finished immediately on the last event, instead of after
    }
};


struct MultByTwoArrow : public JPipelineArrow<MultByTwoArrow> {

    MultByTwoArrow(std::string name, JMailbox<JEvent*>* input_queue, JMailbox<JEvent*>* output_queue) 
        : JPipelineArrow(name, true, false, false) {
        this->set_input(input_queue);
        this->set_output(output_queue);
    }

    void process(JEvent* event, bool& success, JArrowMetrics::Status& status) {
        auto prev = event->Get<EventData>("first");
        auto x = prev.at(0)->x;
        auto next = new EventData { .x=x, .y=x*2.0 };
        event->Insert(next, "second");
        success = true;
        status = JArrowMetrics::Status::KeepGoing;
    }
};

struct SubOneArrow : public JPipelineArrow<SubOneArrow> {

    SubOneArrow(std::string name, JMailbox<JEvent*>* input_queue, JMailbox<JEvent*>* output_queue) 
        : JPipelineArrow(name, true, false, false) {
        this->set_input(input_queue);
        this->set_output(output_queue);
    }

    void process(JEvent* event, bool& success, JArrowMetrics::Status& status) {
        auto prev = event->Get<EventData>("second");
        auto x = prev.at(0)->x;
        auto y = prev.at(0)->y;
        auto z = y - 1;
        auto next = new EventData { .x=x, .y=y, .z=z };
        event->Insert(next, "third");
        success = true;
        status = JArrowMetrics::Status::KeepGoing;
    }
};

struct SumArrow : public JPipelineArrow<SumArrow> {

    double sum = 0;

    SumArrow(std::string name, JMailbox<JEvent*>* input_queue, JEventPool* pool) 
        : JPipelineArrow(name, false, false, true) {
        this->set_input(input_queue);
        this->set_output(pool);
    }

    void process(JEvent* event, bool& success, JArrowMetrics::Status& status) {
        auto prev = event->Get<EventData>("third");
        auto z = prev.at(0)->z;
        sum += z;
        success = true;
        status = JArrowMetrics::Status::KeepGoing;
    }
};


