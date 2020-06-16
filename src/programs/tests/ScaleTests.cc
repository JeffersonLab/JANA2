#include "catch.hpp"

#include <JANA/JApplication.h>
#include <JANA/Utils/JCpuInfo.h>

TEST_CASE("nthreads") {

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
	threads = app.GetNThreads();
	REQUIRE(threads == 8);
}
