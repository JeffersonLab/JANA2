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

TEST_CASE("JApplication::Scale updates the number of workers") {
	JApplication app;
	app.SetParameterValue("nthreads", 4);
	app.Run(false);
	auto threads = app.GetNThreads();
	REQUIRE(threads == 4);

	app.Scale(8);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	// There is a ticker interval which prevents JApplication::update_status from fetching the most up-to-date
	// thread count. Obviously this should be improved, but we'll deal with that later.
	// TODO: Rethink update_status
	threads = app.GetNThreads();
	REQUIRE(threads == 8);
}

TEST_CASE("JApplication::Scale improves the throughput", "[.][performance]") {
    JApplication app;
    app.SetTicker(false);
    app.Add(new DummySource("dummy", &app));
    app.Add(new DummyProcessor);
    app.SetParameterValue("benchmark:minthreads", 1);
    app.SetParameterValue("benchmark:maxthreads", 5);
    app.SetParameterValue("benchmark:threadstep", 2);
    app.SetParameterValue("benchmark:nsamples", 3);
    app.Initialize();
    auto japc = app.GetService<JArrowProcessingController>();
    app.Run(false);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_1 = japc->measure_internal_performance()->latest_throughput_hz;
    std::cout << "nthreads=1: throughput_hz=" << throughput_hz_1 << std::endl;
    app.Scale(2);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_2 = japc->measure_internal_performance()->latest_throughput_hz;
    std::cout << "nthreads=2: throughput_hz=" << throughput_hz_2 << std::endl;
    app.Scale(4);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto throughput_hz_4 = japc->measure_internal_performance()->latest_throughput_hz;
    std::cout << "nthreads=4: throughput_hz=" << throughput_hz_4 << std::endl;
    app.Quit();
    REQUIRE(throughput_hz_2 > throughput_hz_1*1.5);
    REQUIRE(throughput_hz_4 > throughput_hz_2*1.25);
}
