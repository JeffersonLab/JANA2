
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>


TEST_CASE("HallDReconTopology") {
    // 5Hz for full reconstruction
    // 20kHz for stripped-down reconstruction (not per-core)
    // Base memory allcation: 100 MB/core + 600MB
    // 1 thread/event, disentangle 1 event, turn into 40.
    // disentangled (single event size) : 12.5 kB / event (before blown up)
    // entangled "block of 40": dis * 40
    
    auto params = new JParameterManager;
    params->SetParameter("jana:loglevel", "off");

    // Log levels get set as soon as JApp gets constructed
    params->SetParameter("jtest:write_csv", false);
    params->SetParameter("jtest:parser_ms", 2);
    params->SetParameter("jtest:plotter_ms", 2);

    params->SetParameter("benchmark:resultsdir", "perf_tests");
    params->SetParameter("benchmark:rates_filename", "halld_recon_topology.dat");
    params->SetParameter("benchmark:use_log_scale", true);
    params->SetParameter("benchmark:minthreads", "1");
    params->SetParameter("benchmark:maxthreads", "32");

    JApplication app(params);
    auto logger = params->GetLogger("PerfTests");
    app.AddPlugin("JTest");

    LOG_WARN(logger) << "Running JTest tuned to imitate halld_recon" << LOG_END;
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


