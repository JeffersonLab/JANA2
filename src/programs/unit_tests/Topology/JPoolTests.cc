
#include "JANA/JApplicationFwd.h"
#include "JANA/Services/JComponentManager.h"
#include <catch.hpp>
#include <JANA/Topology/JEventPool.h>

namespace jana {
namespace jpooltests {


TEST_CASE("JPoolTests_SingleLocationLimitEvents") {
    JApplication app;
    app.Initialize();
    auto jcm = app.GetService<JComponentManager>();

    JEventPool pool(jcm, 3, 1, true);

    auto* e = pool.get(0);
    REQUIRE(e != nullptr);
    REQUIRE((*e)->GetEventNumber() == 0); // Will segfault if not initialized

    auto* f = pool.get(0);
    REQUIRE(f != nullptr);
    REQUIRE((*f)->GetEventNumber() == 0);

    auto* g = pool.get(0);
    REQUIRE(g != nullptr);
    REQUIRE((*g)->GetEventNumber() == 0);

    auto* h = pool.get(0);
    REQUIRE(h == nullptr);

    (*f)->SetEventNumber(5);
    pool.put(f, true, 0);

    h = pool.get(0);
    REQUIRE(h != nullptr);
    REQUIRE((*h)->GetEventNumber() == 5);
}

TEST_CASE("JPoolTests_SingleLocationUnlimitedEvents") {
    JApplication app;
    app.Initialize();
    auto jcm = app.GetService<JComponentManager>();

    JEventPool pool(jcm, 3, 1, false);

    auto* e = pool.get(0);
    REQUIRE(e != nullptr);
    REQUIRE((*e)->GetEventNumber() == 0);

    auto* f = pool.get(0);
    std::weak_ptr<JEvent> f_weak = *f;
    REQUIRE(f != nullptr);
    REQUIRE((*f)->GetEventNumber() == 0);

    auto* g = pool.get(0);
    std::weak_ptr<JEvent> g_weak = *g;
    REQUIRE(g != nullptr);
    REQUIRE((*g)->GetEventNumber() == 0);

    auto* h = pool.get(0);
    // Unlike the others, h is allocated on the heap
    (*h)->SetEventNumber(9);
    std::weak_ptr<JEvent> h_weak = *h;
    REQUIRE(h != nullptr); 

    (*f)->SetEventNumber(5);
    pool.put(f, true, 0);
    // f goes back into the pool, so dtor does not get called
    REQUIRE(f_weak.lock() != nullptr);

    pool.put(h, true, 0);
    // h's dtor DOES get called
    REQUIRE(h_weak.lock() == nullptr);

    // When we retrieve another event, it comes from the pool
    // So we get what used to be f
    auto* i = pool.get(0);
    REQUIRE(i != nullptr);
    REQUIRE((*i)->GetEventNumber() == 5);
}



} // namespace jana
} // namespace jpooltests
