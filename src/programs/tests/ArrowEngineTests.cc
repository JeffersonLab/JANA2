
#include <JANA/ArrowEngine/Arrow.h>

#include "catch.hpp"

using namespace jana::arrowengine;

TEST_CASE("ArrowEngineTests") {
    StageArrow<int,double>([](int x) { return x + 1.0; });
    REQUIRE (0 == 1);
}

