
#include "catch.hpp"

#include <JANA/Queue.h>


TEST_CASE("Queue: Basic functionality") {
    Queue<int> q;
    q.set_active(true);

    REQUIRE(q.get_item_count() == 0);

    q.push(22);
    REQUIRE(q.get_item_count() == 1);

    std::vector<int> items;
    auto result = q.pop(items, 22);
    REQUIRE(items.size() == 1);
    REQUIRE(q.get_item_count() == 0);
    REQUIRE(result == QueueBase::Status::Empty);

    q.push({1,2,3});
    REQUIRE(q.get_item_count() == 3);

    items.clear();
    result = q.pop(items, 2);
    REQUIRE(items.size() == 2);
    REQUIRE(q.get_item_count() == 1);
    REQUIRE(result == QueueBase::Status::Ready);

}
