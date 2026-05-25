#include "MultiLevelTopologyTests.h"
#include "JANA/Engine/JExecutionEngine.h"
#include "JANA/JApplicationFwd.h"
#include "JANA/JEvent.h"
#include "JANA/JException.h"
#include "JANA/Topology/JArrow.h"
#include "JANA/Topology/JTopologyBuilder.h"
#include "JANA/Utils/JEventLevel.h"

#include <iostream>


namespace jana {
namespace timeslice_tests {


TEST_CASE("TimeslicesTests_FineGrained") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "trace");
    app.SetParameterValue("jana:nevents", "5");
    app.SetParameterValue("jana:max_inflight_timeslices", "2");
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

    auto src_arrow = top->GetArrow("TimesliceSource");
    auto ts_map_arrow = top->GetArrow("TimesliceMap1");
    auto unfold_arrow = top->GetArrow("PhysicsEventUnfold");
    auto pe_map_arrow = top->GetArrow("PhysicsEventMap2");
    auto pe_tap_arrow = top->GetArrow("PhysicsEventTap");

    auto ts_pool = top->GetOrCreatePool(JEventLevel::Timeslice);
    auto pe_pool = top->GetOrCreatePool(JEventLevel::PhysicsEvent);

    auto ts_map_queue = ts_map_arrow->GetPort(0).GetQueue();
    auto unfold_queue = unfold_arrow->GetPort(0).GetQueue();
    auto pe_map_queue = pe_map_arrow->GetPort(0).GetQueue();
    auto pe_tap_queue = pe_tap_arrow->GetPort(0).GetQueue();

    // Test connectivity
    REQUIRE(src_arrow->GetPort(0).GetPool() == ts_pool);
    REQUIRE(src_arrow->GetPort(1).GetQueue() == ts_map_queue);
    REQUIRE(ts_map_arrow->GetPort(1).GetQueue() == unfold_queue);
    REQUIRE(unfold_queue == unfold_arrow->GetPort(JEventLevel::Timeslice, JArrow::PortDirection::In).GetQueue());
    REQUIRE(pe_pool == unfold_arrow->GetPort(JEventLevel::PhysicsEvent, JArrow::PortDirection::In).GetPool());
    REQUIRE(unfold_arrow->GetPort(JEventLevel::Timeslice, JArrow::PortDirection::Out).GetPool() == ts_pool);
    REQUIRE(unfold_arrow->GetPort(JEventLevel::PhysicsEvent, JArrow::PortDirection::Out).GetQueue() == pe_map_queue);
    REQUIRE(pe_map_arrow->GetPort(1).GetQueue() == pe_tap_queue);
    REQUIRE(pe_tap_arrow->GetPort(1).GetPool() == pe_pool);

    JArrow::FireResult result = JArrow::FireResult::NotRunYet;

    result = ee->Fire(src_arrow->GetId(), 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);

    REQUIRE(ts_pool->GetCapacity() == 2);
    REQUIRE(ts_pool->GetSize(0) == 1);
    REQUIRE(ts_map_queue->GetSize(0) == 1);

    result = ee->Fire(ts_map_arrow->GetId(), 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
    REQUIRE(ts_map_queue->GetSize(0) == 0);
    REQUIRE(unfold_queue->GetSize(0) == 1);

    // Parent
    result = ee->Fire(unfold_arrow->GetId(), 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
    
    // Child
    result = ee->Fire(unfold_arrow->GetId(), 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
   
    result = ee->Fire(pe_map_arrow->GetId(), 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
   
    result = ee->Fire(pe_tap_arrow->GetId(), 0);
    REQUIRE(result == JArrow::FireResult::KeepGoing);
    
    REQUIRE(ts_pool->GetSize(0) == 1); // Unfolder still has parent
    REQUIRE(pe_pool->GetSize(0) == 4); // Child returned to pool
    
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

TEST_CASE("TimeslicesTests_NoEvtProcs") {

    JApplication app;
    app.SetParameterValue("jana:nevents", "5");
    app.SetParameterValue("jana:loglevel", "debug");

    app.Add(new MyTimesliceSource);
    app.Add(new MyTimesliceUnfolder);
    //app.Add(new MyEventProcessor);
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


namespace multilevel_source_tests {

TEST_CASE("MultilevelSource_Trivial") {
    // This test case demonstrates the multilevel source behaving just like the plain old JEventSource

    JApplication app;
    auto* source = new MyMultilevelSource;
    auto* proc = new MyMultilevelProcessor;

    source->SetLevel(JEventLevel::PhysicsEvent);
    source->data_stream = {{JEventLevel::PhysicsEvent, 4}, {JEventLevel::PhysicsEvent, 5}, {JEventLevel::PhysicsEvent, 6}};
    proc->expected_data_stream = {{-1,-1,4}, {-1,-1,5}, {-1,-1,6}};

    app.Add(source);
    app.Add(proc);
    app.Run();
}


} // namespace multilevel_source_tests
} // namespce jana






