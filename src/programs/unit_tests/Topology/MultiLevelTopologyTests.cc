#include "MultiLevelTopologyTests.h"
#include "JANA/Engine/JExecutionEngine.h"
#include "JANA/JException.h"
#include "JANA/Topology/JArrow.h"
#include "JANA/Topology/JTopologyBuilder.h"

#include <iostream>
#include <map>

namespace jana {
namespace timeslice_tests {


TEST_CASE("TimeslicesTests_FineGrained") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "trace");
    app.SetParameterValue("jana:nevents", "5");
    app.SetParameterValue("jana:max_inflight_events", "4");

    app.Add(new MyTimesliceSource);
    app.Add(new MyTimesliceUnfolder);
    app.Add(new MyEventProcessor);
    app.Add(new JFactoryGeneratorT<MyProtoClusterFactory>);
    app.Add(new JFactoryGeneratorT<MyClusterFactory>);
    app.SetTicker(true);

    app.Initialize();
    auto ee = app.GetService<JExecutionEngine>();
    auto top = app.GetService<JTopologyBuilder>();
    enum ArrowId {TS_SRC=0, TS_MAP=1, TS_UNF=2, TS_FLD=3, PH_MAP=4, PH_TAP=5};
    JArrow::FireResult result = JArrow::FireResult::NotRunYet;

    result = ee->Fire(TS_SRC, 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);

    REQUIRE(top->arrows[TS_SRC]->get_port(1).queue == top->queues[0]);
    REQUIRE(top->pools[0]->GetCapacity() == 4);
    REQUIRE(top->pools[0]->GetSize(0) == 3);
    REQUIRE(top->queues[0]->GetSize(0) == 1);

    result = ee->Fire(TS_MAP, 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
    REQUIRE(top->queues[0]->GetSize(0) == 0);
    REQUIRE(top->queues[1]->GetSize(0) == 1);
    
    // Parent
    result = ee->Fire(TS_UNF, 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
    
    // Child
    result = ee->Fire(TS_UNF, 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
   
    result = ee->Fire(PH_MAP, 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
   
    result = ee->Fire(PH_TAP, 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
    
    result = ee->Fire(TS_FLD, 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);

    REQUIRE(top->pools[0]->GetSize(0) == 3); // Unfolder still has parent
    REQUIRE(top->pools[1]->GetSize(0) == 4); // Child returned to pool
    
}

TEST_CASE("TimeslicesTests") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "trace");
    app.SetParameterValue("jana:nevents", "5");
    
    app.Add(new MyTimesliceSource);
    app.Add(new MyTimesliceUnfolder);
    app.Add(new MyEventProcessor);
    app.Add(new JFactoryGeneratorT<MyProtoClusterFactory>);
    app.Add(new JFactoryGeneratorT<MyClusterFactory>);
    app.SetTicker(true);
    try {
        app.Run();
    }
    catch (JException& e) {
        std::cout << e << std::endl;
        throw e;
    }
}


} // namespace timeslice_tests
} // namespce jana






