// Copyright 2022-2026, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>


TEST_CASE("MinimalPhysicsEventTopology") {

    JApplication app;

    app.SetParameterValue("jtest:parser:cputime_ms", 0);
    app.SetParameterValue("jtest:parser:cputime_spread", 0);
    app.SetParameterValue("jtest:parser:bytes", 0);
    app.SetParameterValue("jtest:parser:bytes_spread", 0);

    app.SetParameterValue("jtest:disentangler:cputime_ms", 0);
    app.SetParameterValue("jtest:disentangler:cputime_spread", 0);
    app.SetParameterValue("jtest:disentangler:bytes", 0);
    app.SetParameterValue("jtest:disentangler:bytes_spread", 0);

    app.SetParameterValue("jtest:tracker:cputime_ms", 0);
    app.SetParameterValue("jtest:tracker:cputime_spread", 0);
    app.SetParameterValue("jtest:tracker:bytes", 0);
    app.SetParameterValue("jtest:tracker:bytes_spread", 0);

    app.SetParameterValue("jtest:plotter:cputime_ms", 0);
    app.SetParameterValue("jtest:plotter:cputime_spread", 0);
    app.SetParameterValue("jtest:plotter:bytes", 0);
    app.SetParameterValue("jtest:plotter:bytes_spread", 0);

    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "minimal_physics_event_topology.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");
    //params->SetParameter("jana:loglevel", "off");

    auto logger = app.GetJParameterManager()->GetLogger("PerfTests");
    app.AddPlugin("JTest");

    LOG_INFO(logger) << "Running JTest with all sleeps and computations turned off" << LOG_END;
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


