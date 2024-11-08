
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Topology/JMailbox.h>
#include <JANA/Topology/JEventQueue.h>

#include "catch.hpp"


TEST_CASE("QueueTests_Basic") {

    JMailbox<int*> q;
    REQUIRE(q.size() == 0);

    int* item = new int {22};
    bool result = q.try_push(&item, 1, 0);
    REQUIRE(q.size() == 1);
    REQUIRE(result == true);

    int* items[10];
    auto count = q.pop(items, 1, 10, 0);
    REQUIRE(count == 1);
    REQUIRE(q.size() == 0);
    REQUIRE(*(items[0]) == 22);

    *(items[0]) = 33;
    items[1] = new int {44};
    items[2] = new int {55};


    q.push(items, 3, 0);
    REQUIRE(q.size() == 3);

    count = q.pop(items, 2, 2, 0);
    REQUIRE(count == 2);
    REQUIRE(q.size() == 1);
}

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




