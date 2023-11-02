
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


// PerfTests is designed to make it easy to see how changes to JANA's internals affect its performance on
// a variety of workloads we care about.

#include <JANA/JApplication.h>

#include <iostream>
#include <thread>



void benchmark(JApplication& app) {
    app.Run(false);
    std::cout << app.GetIntegratedRate() << std::endl;
    for (int nthreads=1; nthreads<=8; nthreads*=2) {
        app.Scale(nthreads);
        // Warm up for 10 secs
        std::cout << "Warming up (10 secs)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        app.GetInstantaneousRate(); // Reset the clock
        std::cout << "Measuring throughput (20 secs)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(20));
        std::cout << nthreads << "    " << app.GetInstantaneousRate() << std::endl;
    }
    app.Stop(true);
}


int main() {
    
    {
        std::cout << "Running JTest tuned to imitate halld_recon:" << std::endl;

        // 5Hz for full reconstruction
        // 20kHz for stripped-down reconstruction (not per-core)
        // Base memory allcation: 100 MB/core + 600MB
        // 1 thread/event, disentangle 1 event, turn into 40.
        // disentangled (single event size) : 12.5 kB / event (before blown up)
        // entangled "block of 40": dis * 40
        
        auto params = new JParameterManager;
        params->SetParameter("log:off", "JApplication,JPluginLoader,JArrowProcessingController,JArrow"); // Log levels get set as soon as JApp gets constructed XD
        params->SetParameter("jtest:parser_ms", 2);

        JApplication app(params);
        app.AddPlugin("JTest");
        benchmark(app);
    }


    {
        std::cout << "Running JTest with all sleeps and computations turned off" << std::endl;

        auto params = new JParameterManager;
        params->SetParameter("log:off", "JApplication,JPluginLoader,JArrowProcessingController,JArrow"); // Log levels get set as soon as JApp gets constructed XD
                                                                       //
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

        JApplication app(params);
        app.AddPlugin("JTest");
        benchmark(app);
    }
    // Next: Run with PODIO datatypes
    // Next: Run with subevents
    // Next: Run with more and more arrows to see how that scales

}
