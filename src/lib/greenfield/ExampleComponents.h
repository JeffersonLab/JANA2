#pragma once

#include <iostream>

#include <greenfield/Logger.h>
#include <greenfield/Components.h>
#include <thread>

namespace greenfield {

struct RandIntSource : public Source<int> {

    size_t emit_limit = 20;  // How many to emit
    size_t emit_count = 0;   // How many emitted so far
    int emit_sum = 0;        // Sum of all ints emitted so far
    Logger logger;

    Status inprocess(std::vector<int> &items, size_t count) override {

        for (size_t i = 0; i < count && emit_count < emit_limit; ++i) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            int x = 7;
            items.push_back(x);
            emit_count += 1;
            emit_sum += x;
        }
        LOG_TRACE(logger) << "RandIntSource emitted " << emit_count << " events" << LOG_END;

        if (emit_count >= emit_limit) {
            return Status::Finished;
        }
//      else if (emit_count % 5 == 0) {
//          return StreamStatus::ComeBackLater;
//      }
        return Status::KeepGoing;
    }

    void initialize() override {
        LOG_INFO(logger) << "RandIntSource.initialize() called!" << LOG_END;
    };

    void finalize() override {
        LOG_INFO(logger) << "RandIntSource.finalize() called!" << LOG_END;
    }
};


struct MultByTwoProcessor : public ParallelProcessor<int, double> {

    double process(int x) override {
        return x * 2.0;
    }
};


class SubOneProcessor : public ParallelProcessor<double, double> {

private:
    double process(double x) override {
        return x - 1;
    }
};


template<typename T>
struct SumSink : public Sink<T> {

    T sum = 0;
    Logger logger;

    void outprocess(T d) override {
        sum += d;
        LOG_TRACE(logger) << "SumSink.outprocess() called!" << LOG_END;
    }

    void initialize() override {

        LOG_INFO(logger) << "SumSink.initialize() called!" << LOG_END;
    };

    void finalize() override {
        LOG_INFO(logger) << "SumSink.finalize() called!" << LOG_END;
    };
};
}