#pragma once

#include <iostream>

#include <JANA/ArrowEngine/Arrow.h>
#include <JANA/Services/JLoggingService.h>

using namespace jana::arrowengine;

struct RandIntSource : public SourceOp<int> {

    size_t emit_limit = 20;  // How many to emit
    size_t emit_count = 0;   // How many emitted so far
    int emit_sum = 0;        // Sum of all ints emitted so far
    JLogger logger;

    std::pair<Status, int> next() override {
        if (emit_count > emit_limit) {
            return {Status::FailFinished, 0};
        }
        else {
            //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            int x = 7;
            emit_count += 1;
            emit_sum += x;
            return {Status::Success, x};
        }
    }
};


struct MultByTwoProcessor : public MapOp<int, double> {

    double map(int x) override {
        return x * 2.0;
    }
};


struct SubOneProcessor : public MapOp<double, double> {

    double map(double x) override {
        return x - 1;
    }
};


template<typename T>
struct SumSink : public SinkOp<T> {

    T sum = 0;
    JLogger logger;

    void accumulate(T d) override {
        sum += d;
        LOG_DEBUG(logger) << "SumSink.accumulate() called!" << LOG_END;
    }
};


/*
    using namespace jana::arrowengine;

    SourceArrow<int, RandIntSource> source("emit_rand_ints", false);
    StageArrow<int, double, MultByTwoProcessor> p1 ("multiply_by_two", true);
    StageArrow<double, double, SubOneProcessor> p2 ("subtract_one", true);
    SinkArrow<double, SumSink<double>> sink ("sum_everything", false);

 */