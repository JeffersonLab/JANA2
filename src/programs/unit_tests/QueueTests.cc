
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Engine/JMailbox.h>

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

    size_t reserve_count = q.reserve(3, 5, 0);
    REQUIRE(reserve_count == 5);

    q.push_and_unreserve(items, 3, reserve_count, 0);
    REQUIRE(q.size() == 3);

    count = q.pop_and_reserve(items, 2, 2, 0);
    REQUIRE(count == 2);
    REQUIRE(q.size() == 1);
}
