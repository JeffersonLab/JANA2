
#include "catch.hpp"

#include <greenfield/Queue.h>
#include "JServiceLocatorDummies.h"

namespace greenfield {

TEST_CASE("greenfield::Queue: Basic functionality") {
    Queue<int> q;
    REQUIRE(q.get_item_count() == 0);

    q.push(22);
    REQUIRE(q.get_item_count() == 1);

    std::vector<int> items;
    SchedulerHint result = q.pop(items, 22);
    REQUIRE(items.size() == 1);
    REQUIRE(q.get_item_count() == 0);
    REQUIRE(result == SchedulerHint::ComeBackLater);

    q.push({1,2,3});
    REQUIRE(q.get_item_count() == 3);

    items.clear();
    result = q.pop(items, 2);
    REQUIRE(items.size() == 2);
    REQUIRE(q.get_item_count() == 1);
    REQUIRE(result == SchedulerHint::KeepGoing);
    REQUIRE(false == true);

}
}