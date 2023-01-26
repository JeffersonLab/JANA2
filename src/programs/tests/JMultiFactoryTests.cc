
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"
#include <JANA/JEventCallback.h>

class MyCallback : public jana2::JEventCallback {

};

TEST_CASE("MultiFactoryTests") {
    REQUIRE(1 == 0);
}
