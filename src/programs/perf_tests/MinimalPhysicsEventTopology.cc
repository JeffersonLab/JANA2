// Copyright 2022-2026, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>


TEST_CASE("MinimalPhysicsEventTopology") {

    auto params = new JParameterManager;
    params->SetParameter("jana:loglevel", "off");

    // Log levels get set as soon as JApp gets constructed
    params->SetParameter("jtest:write_csv", false);

    params->SetParameter("jtest:parser_ms", 0);
    params->SetParameter("jtest:parser_spread", 0);
    params->SetParameter("jtest:parser_bytes", 0);
    params->SetParameter("jtest:parser_bytes_spread", 0);

    params->SetParameter("jtest:disentangler_ms", 0);
    params->SetParameter("jtest:disentangler_spread", 0);
    params->SetParameter("jtest:disentangler_bytes", 0);
    params->SetParameter("jtest:disentangler_bytes_spread", 0);

    params->SetParameter("jtest:tracker_ms", 0);
    params->SetParameter("jtest:tracker_spread", 0);
    params->SetParameter("jtest:tracker_bytes", 0);
    params->SetParameter("jtest:tracker_bytes_spread", 0);

    params->SetParameter("jtest:plotter_ms", 0);
    params->SetParameter("jtest:plotter_spread", 0);
    params->SetParameter("jtest:plotter_bytes", 0);
    params->SetParameter("jtest:plotter_bytes_spread", 0);

    params->SetParameter("benchmark:resultsdir", "perf_tests");
    params->SetParameter("benchmark:rates_filename", "minimal_physics_event_topology.dat");
    params->SetParameter("benchmark:use_log_scale", true);
    params->SetParameter("benchmark:minthreads", "1");
    params->SetParameter("benchmark:maxthreads", "32");

    JApplication app(params);
    auto logger = params->GetLogger("PerfTests");
    app.AddPlugin("JTest");

    LOG_INFO(logger) << "Running JTest with all sleeps and computations turned off" << LOG_END;
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


