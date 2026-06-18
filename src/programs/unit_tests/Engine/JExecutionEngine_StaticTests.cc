
#include "JANA/Topology/JArrow.h"
#include <memory>
#define JANA2_TESTCASE

#include "JANA/Engine/JExecutionEngine_Static.h"
#include "JANA/Utils/JBenchUtils.h"
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include <chrono>
#include <thread>

namespace jana::staticengine::tests {

struct TestData { int x; };
struct TestSource : public JEventSource {
    JBenchUtils bench;
    TestSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    Result Emit(JEvent& event) override {
        event.Insert<TestData>(new TestData {.x=(int) GetEmittedEventCount() * 2}, "src");
        bench.consume_cpu_ms(100);
        return Result::Success;
    }
};
struct TestProc : public JEventProcessor {
    JBenchUtils bench;
    TestProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }
    void ProcessParallel(const JEvent& event) override {
        JBenchUtils local_bench;
        local_bench.consume_cpu_ms(300);
        auto src_x = event.Get<TestData>("src").at(0)->x;
        event.Insert<TestData>(new TestData {.x=src_x + 1}, "map");
    }
    void ProcessSequential(const JEvent& event) override {
        bench.consume_cpu_ms(200);
        auto map_x = event.Get<TestData>("map").at(0)->x;
        REQUIRE(map_x == (int)event.GetEventNumber()*2 + 1);
    }
};


TEST_CASE("JExecutionEngine_Static_StateMachine") {

    JApplication app;
    app.SetParameterValue("jana:engine", 1);
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut_base = app.GetService<JExecutionEngine>();
    auto sut = std::static_pointer_cast<JExecutionEngine_Static>(sut_base);

    SECTION("ManualFinish") {
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        sut->ScaleWorkers(0);
        sut->RunTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        sut->PauseTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Pausing);

        // Need to trigger the Pause since there are no workers to do so
        auto worker = sut->RegisterWorker("PhysicsEventSource");
        sut->ExchangeTask(worker);

        sut->RunSupervisor();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        sut->FinishTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Finished);
    }

    SECTION("AutoFinish") {
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        sut->ScaleWorkers(0);
        sut->RunTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        sut->PauseTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Pausing);

        // Need to trigger the Pause since there are no workers to do so
        auto worker = sut->RegisterWorker("PhysicsEventSource");
        sut->ExchangeTask(worker);

        sut->RunSupervisor();
        sut->FinishTopology();

        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Finished);
    }
}


TEST_CASE("JExecutionEngine_Static_ExternalWorkers") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 1);
    app.SetParameterValue("jana:max_inflight_events", 2);
    app.SetParameterValue("jana:engine", 1);

    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut_base = app.GetService<JExecutionEngine>();
    auto sut = std::static_pointer_cast<JExecutionEngine_Static>(sut_base);


    SECTION("SelfTermination") {

        auto src = sut->RegisterWorker("PhysicsEventSource");
        auto map = sut->RegisterWorker("PhysicsEventMap1");
        auto tap = sut->RegisterWorker("PhysicsEventTap");


        sut->ExchangeTask(src);
        REQUIRE(src.has_active_task == false);

        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        sut->RunTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);

        sut->ExchangeTask(src);
        REQUIRE(src.has_active_task == true);
        src.Fire();
        REQUIRE(src.output_status == JArrow::FireResult::KeepGoing);
        REQUIRE(src.output_count == 1);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);
        sut->ExchangeTask(src);


        sut->ExchangeTask(map);
        REQUIRE(map.has_active_task == true);
        map.Fire();
        REQUIRE(map.output_status == JArrow::FireResult::KeepGoing);
        REQUIRE(map.output_count == 1);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);
        sut->ExchangeTask(map);


        REQUIRE(src.has_active_task == true);
        src.Fire();
        REQUIRE(src.output_status == JArrow::FireResult::Finished);
        REQUIRE(src.output_count == 1);
        REQUIRE(src.outputs.at(0).second == 0); // Event is returned to the pool
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);

        sut->ExchangeTask(src);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Draining);


        sut->ExchangeTask(tap);
        REQUIRE(tap.has_active_task == true);
        tap.Fire();
        REQUIRE(tap.output_status == JArrow::FireResult::KeepGoing);
        REQUIRE(tap.output_count == 1);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Draining);
        REQUIRE(sut->GetPerf().event_count == 0);

        sut->ExchangeTask(tap);
        REQUIRE(sut->GetPerf().event_count == 1);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);

        sut->RunSupervisor();
        sut->FinishTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Finished);
    }
}


TEST_CASE("JExecutionEngine_Static_ScaleWorkers") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 10);
    app.SetParameterValue("jana:loglevel", "warn");
    app.SetParameterValue("jana:engine", 1);
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();

    SECTION("SingleWorker") {
        REQUIRE(sut->GetPerf().thread_count == 0);
        sut->ScaleWorkers(1);
        REQUIRE(sut->GetPerf().thread_count == 1);
        sut->ScaleWorkers(0);
        REQUIRE(sut->GetPerf().thread_count == 0);
    }
}


TEST_CASE("JExecutionEngine_Static_RunSingleEvent") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 3);
    app.SetParameterValue("jana:loglevel", "debug");
    app.SetParameterValue("jana:engine", 1);
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();

    SECTION("SingleWorker") {
        REQUIRE(sut->GetPerf().thread_count == 0);
        sut->ScaleWorkers(1);
        sut->RunTopology();

        sut->RunSupervisor();
        REQUIRE(sut->GetPerf().thread_count == 1);
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Paused);

        sut->FinishTopology();
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Finished);
        REQUIRE(sut->GetPerf().event_count == 3);

        REQUIRE(sut->GetPerf().thread_count == 1);
        sut->ScaleWorkers(0);
        REQUIRE(sut->GetPerf().thread_count == 0);
    }
    
    SECTION("MultipleWorker") {
        REQUIRE(sut->GetPerf().thread_count == 0);
        sut->ScaleWorkers(4);
        REQUIRE(sut->GetPerf().thread_count == 4);

        sut->RunTopology();

        sut->RunSupervisor();
        REQUIRE(sut->GetPerf().thread_count == 4);
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Paused);

        sut->FinishTopology();
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Finished);
        REQUIRE(sut->GetPerf().event_count == 3);

        REQUIRE(sut->GetPerf().thread_count == 4);
        sut->ScaleWorkers(0);
        REQUIRE(sut->GetPerf().thread_count == 0);
    }
}

TEST_CASE("JExecutionEngine_Static_ExternalPause_Immediate") {
    JApplication app;
    //app.SetParameterValue("jana:loglevel", "trace");
    //app.SetParameterValue("jana:backoff_interval", 100);
    app.SetParameterValue("jana:max_inflight_events", 4);
    app.SetParameterValue("jana:engine", 1);
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();

    REQUIRE(sut->GetPerf().thread_count == 0);
    sut->ScaleWorkers(4);
    REQUIRE(sut->GetPerf().thread_count == 4);

    sut->RunTopology();
    sut->PauseTopology();
    sut->RunSupervisor();

    auto perf = sut->GetPerf();
    REQUIRE(perf.thread_count == 4);
    REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Paused);

    sut->FinishTopology();

    perf = sut->GetPerf();
    REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Finished);
    REQUIRE(perf.event_count == 0);
    REQUIRE(perf.thread_count == 4);

    sut->ScaleWorkers(0);

    perf = sut->GetPerf();
    REQUIRE(perf.thread_count == 0);
}


TEST_CASE("JExecutionEngine_Static_ExternalPause_Delayed") {
    JApplication app;
    //app.SetParameterValue("jana:loglevel", "trace");
    //app.SetParameterValue("jana:backoff_interval", 100);
    app.SetParameterValue("jana:engine", 1);
    app.SetParameterValue("jana:max_inflight_events", 4);
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();

    REQUIRE(sut->GetPerf().thread_count == 0);
    sut->ScaleWorkers(4);
    REQUIRE(sut->GetPerf().thread_count == 4);

    sut->RunTopology();
    std::thread t([&](){ 
        std::this_thread::sleep_for(std::chrono::seconds(3)); 
        sut->PauseTopology();
    });

    sut->RunSupervisor();
    t.join();

    auto perf = sut->GetPerf();
    REQUIRE(perf.thread_count == 4);
    REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Paused);
    REQUIRE(perf.event_count > 5);

    sut->ScaleWorkers(0);
    REQUIRE(sut->GetPerf().thread_count == 0);
}


TEST_CASE("JExecutionEngine_Static_MaxInFlightEvents") {
    JApplication app;
    auto src = new JEventSource;
    src->SetCallbackStyle(JEventProcessor::CallbackStyle::ExpertMode);
    app.Add(src);
    auto proc = new JEventProcessor;
    proc->SetCallbackStyle(JEventProcessor::CallbackStyle::ExpertMode);
    app.Add(proc);
    app.SetParameterValue("nthreads", 4);
    app.SetParameterValue("jana:loglevel", "trace");
    app.SetParameterValue("jana:nevents", 500);
    app.SetParameterValue("jana:engine", 1);
    // Should give us 6 physics events in our pool

    app.Initialize();
    auto top = app.GetService<JTopologyBuilder>();
    REQUIRE(top->GetPools().size() == 1);
    REQUIRE(top->GetPools().at(0)->GetCapacity() == 6);
    REQUIRE(top->GetPools().at(0)->GetSize(0) == 6);
    app.Run();

}

} // jana::staticengine::tests



