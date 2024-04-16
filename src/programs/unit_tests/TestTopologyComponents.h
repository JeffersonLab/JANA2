
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <iostream>

#include <JANA/Services/JLoggingService.h>
#include <JANA/Topology/JPipelineArrow.h>
#include <thread>
#include <MapArrow.h>


struct RandIntSource : public JPipelineArrow<RandIntSource, int> {

    size_t emit_limit = 20;  // How many to emit
    size_t emit_count = 0;   // How many emitted so far
    int emit_sum = 0;        // Sum of all ints emitted so far
    JLogger logger;

    RandIntSource(std::string name, JPool<int>* pool, JMailbox<int*>* output_queue)
        : JPipelineArrow<RandIntSource, int>(name, false, true, false, nullptr, output_queue, pool) {}

    void process(int* item, bool& success, JArrowMetrics::Status& status) {

        if (emit_count >= emit_limit) {
            success = false;
            status = JArrowMetrics::Status::Finished;
            return;
        }
        *item = 7;
        emit_sum += *item;
        emit_count += 1;
        LOG_DEBUG(logger) << "RandIntSource emitted event " << emit_count << " with value " << *item << LOG_END;
        success = true;
        status = (emit_count == emit_limit) ? JArrowMetrics::Status::Finished : JArrowMetrics::Status::KeepGoing;
        // This design lets us declare Finished immediately on the last event, instead of after
    }

    void initialize() override {
        LOG_INFO(logger) << "RandIntSource.initialize() called!" << LOG_END;
    };

    void finalize() override {
        LOG_INFO(logger) << "RandIntSource.finalize() called!" << LOG_END;
    }
};


struct MultByTwoProcessor : public ParallelProcessor<int*, double*> {

    double* process(int* x) override {
        return new double(*x * 2.0);
    }
};


struct SubOneProcessor : public JPipelineArrow<SubOneProcessor, double> {

    SubOneProcessor(std::string name, JMailbox<double*>* input_queue, JMailbox<double*>* output_queue) 
        : JPipelineArrow<SubOneProcessor, double>(name, true, false, false, input_queue, output_queue, nullptr) {}

    void process(double* item, bool&, JArrowMetrics::Status&) {
        *item -= 1;
    }
};


template<typename T>
struct SumSink : public JPipelineArrow<SumSink<T>, T> {

    T sum = 0;

    SumSink(std::string name, JMailbox<T*>* input_queue, JPool<T>* pool) 
        : JPipelineArrow<SumSink<T>,T>(name, false, false, true, input_queue, nullptr, pool) {}

    void process(T* item, bool&, JArrowMetrics::Status&) {
        sum += *item;
        LOG_DEBUG(JArrow::m_logger) << "SumSink.outprocess() called!" << LOG_END;
    }

    void initialize() override {
        LOG_INFO(JArrow::m_logger) << "SumSink.initialize() called!" << LOG_END;
    };

    void finalize() override {
        LOG_INFO(JArrow::m_logger) << "SumSink.finalize() called!" << LOG_END;
    };
};


