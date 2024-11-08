
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Topology/JEventQueue.h>

#include "catch.hpp"


TEST_CASE("JEventQueueTests_Basic") {

    JEventQueue sut(2,1);

    JEvent event1;
    event1.SetEventNumber(1);

    JEvent event2;
    event2.SetEventNumber(2);

    JEvent event3;
    event3.SetEventNumber(3);

    REQUIRE(sut.GetCapacity() == 2);
    REQUIRE(sut.GetSize(0) == 0);

    sut.Push(&event1, 0);
    REQUIRE(sut.GetSize(0) == 1);

    JEvent* e = sut.Pop(0);
    REQUIRE(sut.GetSize(0) == 0);

    e = sut.Pop(0);
    REQUIRE(e == nullptr);

    sut.Push(&event3, 0);
    sut.Push(&event2, 0);
    REQUIRE(sut.GetSize(0) == 2);

    e = sut.Pop(0);
    REQUIRE(e->GetEventNumber() == 3);
    REQUIRE(sut.GetSize(0) == 1);

    sut.Push(&event1, 0);
    REQUIRE_THROWS(sut.Push(&event1, 0));
}




