
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <JANA/JObject.h>

class SillyObject : public JObject {
public:
    JOBJECT_PUBLIC(SillyObject)
};

TEST_CASE("JObject::className") {
    SillyObject sut;
    REQUIRE(sut.static_className() == "SillyObject");
    REQUIRE(sut.className() == "SillyObject");
}

