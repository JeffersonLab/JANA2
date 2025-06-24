
#define JANA2_TESTCASE

#include "JANA/Engine/JExecutionEngine.h"
#include "JANA/Utils/JBenchUtils.h"
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>
#include <chrono>
#include <thread>

namespace jana::engine::tests {

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


TEST_CASE("JExecutionEngine_StateMachine") {

    JApplication app;
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();

    SECTION("ManualFinish") {
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        sut->ScaleWorkers(0);
        sut->RunTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        sut->PauseTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Pausing);
        
        // Need to trigger the Pause since there are no workers to do so
        auto worker = sut->RegisterWorker();
        JExecutionEngine::Task task;
        sut->ExchangeTask(task, worker.worker_id, true);

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
        auto worker = sut->RegisterWorker();
        JExecutionEngine::Task task;
        sut->ExchangeTask(task, worker.worker_id, true);

        sut->RunSupervisor();
        sut->FinishTopology();

        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Finished);
    }
}


TEST_CASE("JExecutionEngine_ExternalWorkers") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 1);
    app.SetParameterValue("jana:max_inflight_events", 2);

    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();


    SECTION("SelfTermination") {

        auto worker = sut->RegisterWorker();
        REQUIRE(worker.worker_id == 0); // Not threadsafe doing this

        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        REQUIRE(sut->GetPerf().thread_count == 1);
        sut->RunTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);

        JExecutionEngine::Task task;

        sut->ExchangeTask(task, worker.worker_id);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventSource"); // Only task available at this point!
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.status == JArrow::FireResult::KeepGoing);

        sut->ExchangeTask(task, worker.worker_id);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventSource"); // This will fail due to jana:nevents
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.outputs[0].second == 0); // Failure => return to pool
        REQUIRE(task.status == JArrow::FireResult::Finished);

        sut->ExchangeTask(task, worker.worker_id);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventMap2");
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Draining);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.status == JArrow::FireResult::KeepGoing);

        sut->ExchangeTask(task, worker.worker_id);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventTap");
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Draining);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.status == JArrow::FireResult::KeepGoing);

        sut->ExchangeTask(task, worker.worker_id, true);
        REQUIRE(task.arrow == nullptr);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        REQUIRE(sut->GetPerf().event_count == 1);

        sut->RunSupervisor();
        sut->FinishTopology();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Finished);
    }
}


TEST_CASE("JExecutionEngine_ScaleWorkers") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 10);
    app.SetParameterValue("jana:loglevel", "debug");
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


TEST_CASE("JExecutionEngine_RunSingleEvent") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 3);
    app.SetParameterValue("jana:loglevel", "debug");
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

TEST_CASE("JExecutionEngine_ExternalPause") {
    JApplication app;
    app.SetParameterValue("jana:loglevel", "info");
    app.SetParameterValue("jana:max_inflight_events", 4);
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();

    REQUIRE(sut->GetPerf().thread_count == 0);
    sut->ScaleWorkers(4);
    REQUIRE(sut->GetPerf().thread_count == 4);

    SECTION("PauseImmediately") {
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

    SECTION("RunForABit") {
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
}

} // jana::engine::tests



