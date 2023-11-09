
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


// PerfTests is designed to make it easy to see how changes to JANA's internals affect its performance on
// a variety of workloads we care about.

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#if HAVE_PODIO
#include <PodioStressTest.h>
#endif

#include <iostream>
#include <thread>



int main() {
    
    {

        // 5Hz for full reconstruction
        // 20kHz for stripped-down reconstruction (not per-core)
        // Base memory allcation: 100 MB/core + 600MB
        // 1 thread/event, disentangle 1 event, turn into 40.
        // disentangled (single event size) : 12.5 kB / event (before blown up)
        // entangled "block of 40": dis * 40
        
        auto params = new JParameterManager;
        params->SetParameter("log:off", "JApplication,JPluginLoader,JArrowProcessingController,JArrow"); // Log levels get set as soon as JApp gets constructed XD
        params->SetParameter("jtest:write_csv", false);
        params->SetParameter("jtest:parser_ms", 2);

        params->SetParameter("benchmark:resultsdir", "perftest_fake_halldrecon");

        JApplication app(params);
        auto logger = app.GetService<JLoggingService>()->get_logger("PerfTests");
        app.AddPlugin("JTest");

        LOG_INFO(logger) << "Running JTest tuned to imitate halld_recon" << LOG_END;
        JBenchmarker benchmarker(&app);
        benchmarker.RunUntilFinished();
    }


    {

        auto params = new JParameterManager;
        params->SetParameter("log:off", "JApplication,JPluginLoader,JArrowProcessingController,JArrow"); // Log levels get set as soon as JApp gets constructed XD
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

        params->SetParameter("benchmark:resultsdir", "perftest_pure_overhead");

        JApplication app(params);
        auto logger = app.GetService<JLoggingService>()->get_logger("PerfTests");
        app.AddPlugin("JTest");

        LOG_INFO(logger) << "Running JTest with all sleeps and computations turned off" << LOG_END;
        JBenchmarker benchmarker(&app);
        benchmarker.RunUntilFinished();
    }

#if HAVE_PODIO
    {
        // Test that we can link against PODIO datamodel
        // TODO: Delete me 
        ExampleHitCollection c;

        auto params = new JParameterManager;
        params->SetParameter("log:off", "JApplication,JPluginLoader,JArrowProcessingController,JArrow"); // Log levels get set as soon as JApp gets constructed XD
        JApplication app(params);
        auto logger = app.GetService<JLoggingService>()->get_logger("PerfTests");
        // TODO: Add Podio sources, processors, and factories just like JTest
        LOG_INFO(logger) << "Running PODIO stress test" << LOG_END;
        benchmark(app);
    }
#endif

    // Next: Run with subevents
    // Next: Run with more and more arrows to see how that scales
    // Next: Barrier events

}
