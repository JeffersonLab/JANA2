
#include <JANA/JMailbox.h>

#include "catch.hpp"


TEST_CASE("Queue: Basic functionality") {
    JMailbox<int> q;
    q.set_active(true);

    REQUIRE(q.size() == 0);

    int item = 22;
    q.push(item, 0);
    REQUIRE(q.size() == 1);

    std::vector<int> items;
    auto result = q.pop(items, 22);
    REQUIRE(items.size() == 1);
    REQUIRE(q.size() == 0);
    REQUIRE(result == JMailbox<int>::Status::Empty);

    std::vector<int> buffer {1,2,3};
    q.push(buffer, 0);
    REQUIRE(q.size() == 3);
    REQUIRE(buffer.size() == 0);

    items.clear();
    result = q.pop(items, 2);
    REQUIRE(items.size() == 2);
    REQUIRE(q.size() == 1);
    REQUIRE(result == JMailbox<int>::Status::Ready);

}
