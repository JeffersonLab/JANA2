
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
    void Process(const JEvent& event) override {
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
        sut->Scale(0);
        sut->Run();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        sut->RequestPause();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Pausing);
        
        // Need to trigger the Pause since there are no workers to do so
        auto worker_id = sut->RegisterExternalWorker();
        JExecutionEngine::Task task;
        task.worker_id = worker_id;
        sut->ExchangeTask(task, true);

        sut->Wait();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        sut->Finish();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Finished);
    }

    SECTION("AutoFinish") {
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        sut->Scale(0);
        sut->Run();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        sut->RequestPause();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Pausing);

        // Need to trigger the Pause since there are no workers to do so
        auto worker_id = sut->RegisterExternalWorker();
        JExecutionEngine::Task task;
        task.worker_id = worker_id;
        sut->ExchangeTask(task, true);

        sut->Wait(true);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Finished);
    }
}


TEST_CASE("JExecutionEngine_ExternalWorkers") {
    JApplication app;
    app.SetParameterValue("jana:nevents", 1);
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();


    SECTION("SelfTermination") {

        auto worker_id = sut->RegisterExternalWorker();
        REQUIRE(worker_id == 0); // Not threadsafe doing this

        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        REQUIRE(sut->GetPerf().thread_count == 1);
        sut->Run();
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);

        JExecutionEngine::Task task;
        task.worker_id = worker_id;

        sut->ExchangeTask(task);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventSource"); // Only task available at this point!
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.status == JArrowMetrics::Status::KeepGoing);

        sut->ExchangeTask(task);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventSource"); // This will fail due to jana:nevents
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.outputs[0].second == 0); // Failure => return to pool
        REQUIRE(task.status == JArrowMetrics::Status::Finished);

        sut->ExchangeTask(task);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventMap2");
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.status == JArrowMetrics::Status::KeepGoing);

        sut->ExchangeTask(task);
        REQUIRE(task.arrow != nullptr);
        REQUIRE(task.arrow->get_name() == "PhysicsEventTap");
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Running);
        REQUIRE(sut->GetPerf().event_count == 0);

        task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        REQUIRE(task.output_count == 1);
        REQUIRE(task.status == JArrowMetrics::Status::KeepGoing);

        sut->ExchangeTask(task, true);
        REQUIRE(task.arrow == nullptr);
        REQUIRE(sut->GetRunStatus() == JExecutionEngine::RunStatus::Paused);
        REQUIRE(sut->GetPerf().event_count == 1);

        sut->Wait(true);
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
        sut->Scale(1);
        REQUIRE(sut->GetPerf().thread_count == 1);
        sut->Scale(0);
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
        sut->Scale(1);
        sut->Run();

        sut->Wait();
        REQUIRE(sut->GetPerf().thread_count == 1);
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Paused);

        sut->Finish();
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Finished);
        REQUIRE(sut->GetPerf().event_count == 3);

        REQUIRE(sut->GetPerf().thread_count == 1);
        sut->Scale(0);
        REQUIRE(sut->GetPerf().thread_count == 0);
    }
    
    SECTION("MultipleWorker") {
        REQUIRE(sut->GetPerf().thread_count == 0);
        sut->Scale(4);
        REQUIRE(sut->GetPerf().thread_count == 4);

        sut->Run();

        sut->Wait();
        REQUIRE(sut->GetPerf().thread_count == 4);
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Paused);

        sut->Finish();
        REQUIRE(sut->GetPerf().runstatus == JExecutionEngine::RunStatus::Finished);
        REQUIRE(sut->GetPerf().event_count == 3);

        REQUIRE(sut->GetPerf().thread_count == 4);
        sut->Scale(0);
        REQUIRE(sut->GetPerf().thread_count == 0);
    }
}

TEST_CASE("JExecutionEngine_ExternalPause") {
    JApplication app;
    app.SetParameterValue("jana:loglevel", "info");
    app.Add(new TestSource());
    app.Add(new TestProc());
    app.Initialize();
    auto sut = app.GetService<JExecutionEngine>();

    REQUIRE(sut->GetPerf().thread_count == 0);
    sut->Scale(4);
    REQUIRE(sut->GetPerf().thread_count == 4);

    SECTION("PauseImmediately") {
        sut->Run();
        sut->RequestPause();
        sut->Wait();

        auto perf = sut->GetPerf();
        REQUIRE(perf.thread_count == 4);
        REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Paused);

        sut->Finish();

        perf = sut->GetPerf();
        REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Finished);
        REQUIRE(perf.event_count == 0);
        REQUIRE(perf.thread_count == 4);

        sut->Scale(0);

        perf = sut->GetPerf();
        REQUIRE(perf.thread_count == 0);
    }

    SECTION("RunForABit") {
        sut->Run();
        std::thread t([&](){ 
            std::this_thread::sleep_for(std::chrono::seconds(3)); 
            sut->RequestPause();
        });

        sut->Wait();
        t.join();

        auto perf = sut->GetPerf();
        REQUIRE(perf.thread_count == 4);
        REQUIRE(perf.runstatus == JExecutionEngine::RunStatus::Paused);
        REQUIRE(perf.event_count > 5);

        sut->Scale(0);
        REQUIRE(sut->GetPerf().thread_count == 0);
    }
}

} // jana::engine::tests



