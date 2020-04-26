
#include <JANA/ArrowEngine/Arrow.h>

#include "catch.hpp"

using namespace jana::arrowengine;

TEST_CASE("ArrowEngineTests") {
    double sum = 0.0;

    SourceArrow<int> zeros ([]() { return 0; });
    StageArrow<int,double> add_one ([](int x) { return x + 1.0; });
    SinkArrow<double> sum_sink ([&sum](double d) { sum += d; });

    attach(zeros, add_one);
    attach(add_one, sum_sink);

    zeros.execute();
    add_one.execute();
    sum_sink.execute();

    REQUIRE (sum == 1.0);
}

