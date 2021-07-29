#include "catch.hpp"

#include <JANA/JApplication.h>
#include <JANA/Utils/JCpuInfo.h>

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

TEST_CASE("JApplication::Scale") {
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
