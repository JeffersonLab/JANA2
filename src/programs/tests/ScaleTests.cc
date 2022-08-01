#include "catch.hpp"

#include <JANA/JApplication.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Engine/JArrowProcessingController.h>

#include "ScaleTests.h"

TEST_CASE("NThreads") {

    JApplication app;
    SECTION("If nthreads not provided, default to 1") {
        app.Run(true);
        auto threads = app.GetNThreads();
        REQUIRE(threads == 1);
    }

    SECTION("If nthreads=Ncores, use ncores") {
        auto ncores = JCpuInfo::GetNumCpus();
        app.SetParameterValue("nthreads", "Ncores");
        app.Run(true);
        auto threads = app.GetNThreads();
        REQUIRE(threads == ncores);
    }

    SECTION("If nthreads is something else, use that") {
        app.SetParameterValue("nthreads", 17);
        app.Run(true);
        auto threads = app.GetNThreads();
        REQUIRE(threads == 17);
    }
}

TEST_CASE("ScaleNWorkerUpdate") {
    auto params = new JParameterManager();
    // params->SetParameter("log:debug", "JWorker,JArrowTopology,JScheduler,JArrow");
    JApplication app(params);
    app.Add(new scaletest::DummySource("DummySource", &app));
    app.Add(new scaletest::DummyProcessor);
    app.SetParameterValue("nthreads", 4);
    app.Run(false);
    auto threads = app.GetNThreads();
    REQUIRE(threads == 4);

    app.Scale(8); // Scale blocks until workers have fired up

    // Ideally we could just do this:
    // threads = app.GetNThreads();
    // However, we can't, because JApplication caches performance metrics based off of a ticker interval, and
    // Scale() doesn't invalidate the cache. We don't have a clean mechanism to manually force a cache invalidation
    // from JApplication yet. So for now we will obtain the thread count directly from the JProcessingController.

    auto pc = app.GetService<JProcessingController>();
    auto perf_summary = pc->measure_performance();
    threads = perf_summary->thread_count;

    REQUIRE(threads == 8);
    app.Quit();
}

TEST_CASE("ScaleThroughputImprovement", "[.][performance]") {

    auto parms = new JParameterManager;
    // parms->SetParameter("log:debug","JArrowProcessingController,JWorker,JArrow");
    // parms->SetParameter("log:info","JScheduler");
    JApplication app(parms);
    app.SetTicker(false);
    app.Add(new scaletest::DummySource("dummy", &app));
    app.Add(new scaletest::DummyProcessor);
    // app.SetParameterValue("benchmark:minthreads", 1);
    // app.SetParameterValue("benchmark:maxthreads", 5);
    // app.SetParameterValue("benchmark:threadstep", 2);
    // app.SetParameterValue("benchmark:nsamples", 3);
    app.Initialize();
    auto japc = app.GetService<JArrowProcessingController>();
    app.Run(false);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_1 = japc->measure_internal_performance()->latest_throughput_hz;
    japc->print_report();
    std::cout << "nthreads=1: throughput_hz=" << throughput_hz_1 << std::endl;
    app.Scale(2);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_2 = japc->measure_internal_performance()->latest_throughput_hz;
    japc->print_report();
    std::cout << "nthreads=2: throughput_hz=" << throughput_hz_2 << std::endl;
    app.Scale(4);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_4 = japc->measure_internal_performance()->latest_throughput_hz;
    japc->print_report();
    std::cout << "nthreads=4: throughput_hz=" << throughput_hz_4 << std::endl;
    app.Quit();
    REQUIRE(throughput_hz_2 > throughput_hz_1*1.5);
    REQUIRE(throughput_hz_4 > throughput_hz_2*1.25);
}
