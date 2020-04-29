
#include <JANA/ArrowEngine/Arrow.h>

#include "catch.hpp"

using namespace jana::arrowengine;

TEST_CASE("ArrowEngineTests") {
    double sum = 0.0;

    using Status = SourceArrow<int>::Status;
    SourceArrow<int> zeros ("zeros", []() -> std::pair<Status,int> { return std::make_pair(Status::Success,0); });
    StageArrow<int,double> add_one ("add_one", [](int x) { return x + 1.0; }, false);
    SinkArrow<double> sum_sink ("sum_sink", [&sum](double d) { sum += d; }, false);

    zeros.chunk_size = 1;

    attach(zeros, add_one);
    attach(add_one, sum_sink);

    JArrowMetrics m;
    zeros.execute(m, 0);
    add_one.execute(m, 0);
    sum_sink.execute(m, 0);

    REQUIRE (sum == 1.0);
}

