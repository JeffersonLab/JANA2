
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

    JEventPool pool(jcm, 3, 1);

    auto* e = pool.Pop(0);
    REQUIRE(e != nullptr);
    REQUIRE(e->GetEventNumber() == 0); // Will segfault if not initialized

    auto* f = pool.Pop(0);
    REQUIRE(f != nullptr);
    REQUIRE(f->GetEventNumber() == 0);

    auto* g = pool.Pop(0);
    REQUIRE(g != nullptr);
    REQUIRE(g->GetEventNumber() == 0);

    auto* h = pool.Pop(0);
    REQUIRE(h == nullptr);

    f->SetEventNumber(5);
    pool.Push(f, 0);

    h = pool.Pop(0);
    REQUIRE(h != nullptr);
    REQUIRE(h->GetEventNumber() == 5);
}



} // namespace jana
} // namespace jpooltests


