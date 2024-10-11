
#include <catch.hpp>
#include <JANA/Topology/JPool.h>

namespace jana {
namespace jpooltests {

struct Event {
    int x=3;
    double y=7.8;
    bool* was_dtor_called = nullptr;

    ~Event() {
        if (was_dtor_called != nullptr) {
            *was_dtor_called = true;
        }
    }
};

TEST_CASE("JPoolTests_SingleLocationLimitEvents") {

    JPool<Event> pool(3, 1, true);
    pool.init();

    Event* e = pool.get(0);
    REQUIRE(e != nullptr);
    REQUIRE(e->x == 3);

    Event* f = pool.get(0);
    REQUIRE(f != nullptr);
    REQUIRE(f->x == 3);

    Event* g = pool.get(0);
    REQUIRE(g != nullptr);
    REQUIRE(g->x == 3);

    Event* h = pool.get(0);
    REQUIRE(h == nullptr);

    f->x = 5;
    pool.put(f, true, 0);

    h = pool.get(0);
    REQUIRE(h != nullptr);
    REQUIRE(h->x == 5);
}

TEST_CASE("JPoolTests_SingleLocationUnlimitedEvents") {

    bool was_dtor_called = false;
    JPool<Event> pool(3, 1, false);
    pool.init();

    Event* e = pool.get(0);
    e->was_dtor_called = &was_dtor_called;
    REQUIRE(e != nullptr);
    REQUIRE(e->x == 3);

    Event* f = pool.get(0);
    f->was_dtor_called = &was_dtor_called;
    REQUIRE(f != nullptr);
    REQUIRE(f->x == 3);

    Event* g = pool.get(0);
    g->was_dtor_called = &was_dtor_called;
    REQUIRE(g != nullptr);
    REQUIRE(g->x == 3);

    Event* h = pool.get(0);
    // Unlike the others, h is allocated on the heap
    h->x = 9;
    h->was_dtor_called = &was_dtor_called;
    REQUIRE(h != nullptr); 
    REQUIRE(g->x == 3);

    f->x = 5;
    pool.put(f, true, 0);
    // f goes back into the pool, so dtor does not get called
    REQUIRE(was_dtor_called == false);

    pool.put(h, true, 0);
    // h's dtor DOES get called
    REQUIRE(was_dtor_called == true);

    // When we retrieve another event, it comes from the pool
    // So we get what used to be f
    Event* i = pool.get(0);
    REQUIRE(i != nullptr);
    REQUIRE(i->x == 5);
}



} // namespace jana
} // namespace jpooltests
