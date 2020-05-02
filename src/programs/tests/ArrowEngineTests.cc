
#include <JANA/ArrowEngine/Arrow.h>

#include "catch.hpp"

using namespace jana::arrowengine;


struct IntegerRange : public SourceOp<int> {

    int current = 1;
    int max = 10;

    IntegerRange(int min, int max) : current(min), max(max) {};

    std::pair<Status, int> next() override {
        if (current > max) {
            return {Status::FailFinished, current};
        }
        else {
            return {Status::Success, current++};
        }
    }
};


struct AddOneStage : public MapOp<int, double> {
    double map(int x) override {
        return x+1.0;
    }
};


struct SumSink : public SinkOp<double> {

    double acc = 0.0;

    void accumulate(double item) override {
        acc += item;
    }
};


TEST_CASE("ArrowEngineTests") {

    SourceArrow<int, IntegerRange> range_source ("range", false, 1, 10);
    // Problems: arrow name and parallelism.
    // Options: 1. pass as ctor args
    //          2. put on "op" class

    StageArrow<int,double,AddOneStage> add_one_stage ("add_one", false);
    SinkArrow<double,SumSink> sum_sink ("sum_sink", false);

    range_source.chunk_size = 1;

    attach(range_source, add_one_stage);
    attach(add_one_stage, sum_sink);

    JArrowMetrics m;
    range_source.execute(m, 0);
    add_one_stage.execute(m, 0);
    sum_sink.execute(m, 0);

    REQUIRE (sum_sink.acc == 2.0);
}

