#include "catch.hpp"

#include <JANA/JApplication.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Engine/JExecutionEngine.h>

#include "ScaleTests.h"

TEST_CASE("MissingEventSource") {
    JApplication app;
    app.Initialize();
    // Initialize succeeds
    try {
        app.Run();
        // Run() throws
        REQUIRE(0 == 1);
    }
    catch (JException& e) {
        REQUIRE(e.GetMessage() == "Cannot execute an empty topology! Hint: Have you provided an event source?");
    }
}

TEST_CASE("NThreads") {

    JApplication app;
    app.SetParameterValue("jana:nevents",3);
    app.SetParameterValue("jana:loglevel","warn");
    app.Add(new scaletest::DummySource);

    SECTION("If nthreads not provided, default to 1") {
        app.Run(false);
        auto threads = app.GetNThreads();
        REQUIRE(threads == 1);
        app.Stop(true);
    }

    SECTION("If nthreads=Ncores, use ncores") {
        auto ncores = JCpuInfo::GetNumCpus();
        app.SetParameterValue("nthreads", "Ncores");
        app.Run(false);
        auto threads = app.GetNThreads();
        REQUIRE(threads == ncores);
        app.Stop(true);
    }

    SECTION("If nthreads is something else, use that") {
        app.SetParameterValue("nthreads", 17);
        app.Run(false);
        auto threads = app.GetNThreads();
        REQUIRE(threads == 17);
        app.Stop(true);
    }
}

TEST_CASE("ScaleNWorkerUpdate") {
    JApplication app;
    app.SetParameterValue("nthreads",4);
    app.SetParameterValue("jana:loglevel","warn");
    app.Add(new scaletest::DummySource);
    app.Add(new scaletest::DummyProcessor);
    app.Add(new JFactoryGeneratorT<scaletest::DummyFactory>());
    app.Run(false);
    auto threads = app.GetNThreads();
    REQUIRE(threads == 4);

    app.Stop(true, false);

    app.Scale(8); // Scale blocks until workers have fired up
    threads = app.GetNThreads();
    REQUIRE(threads == 8);
    app.Stop(true);
}

TEST_CASE("ScaleThroughputImprovement", "[.][performance]") {

    JApplication app;
    app.SetParameterValue("jana:loglevel", "INFO");
    //app.SetTicker(false);
    app.Add(new scaletest::DummySource);
    app.Add(new scaletest::DummyProcessor);
    app.Add(new JFactoryGeneratorT<scaletest::DummyFactory>());
    app.Initialize();
    auto jee = app.GetService<JExecutionEngine>();

    jee->ScaleWorkers(1);
    jee->RunTopology();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_1 = jee->GetPerf().throughput_hz;
    std::cout << "nthreads=1: throughput_hz=" << throughput_hz_1 << std::endl;
    jee->PauseTopology();
    jee->RunSupervisor();

    jee->ScaleWorkers(2);
    jee->RunTopology();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_2 = jee->GetPerf().throughput_hz;
    std::cout << "nthreads=2: throughput_hz=" << throughput_hz_2 << std::endl;
    REQUIRE(jee->GetPerf().runstatus == JExecutionEngine::RunStatus::Running);
    jee->PauseTopology();
    jee->RunSupervisor();

    jee->ScaleWorkers(4);
    jee->RunTopology();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_4 = jee->GetPerf().throughput_hz;
    jee->PauseTopology();
    jee->RunSupervisor();
    std::cout << "nthreads=4: throughput_hz=" << throughput_hz_4 << std::endl;

    jee->FinishTopology();

    REQUIRE(throughput_hz_2 > throughput_hz_1*1.5);
    REQUIRE(throughput_hz_4 > throughput_hz_2*1.25);
}
