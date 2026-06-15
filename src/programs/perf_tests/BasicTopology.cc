// Copyright 2022-2026, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>


namespace jana::perftest::basic {

struct Data { size_t x; };

struct PESrc : public JEventSource {

    Output<Data> data_out {this};

    PESrc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_out.SetShortName("1");
    }
    JEventSource::Result Emit(JEvent& event) override {
        data_out().push_back(new Data {event.GetEventNumber()*3 });
        return Result::Success;
    };
};

struct PEFac : public JFactory {
    Input<Data> data_in {this};
    Output<Data> data_out {this};
    PEFac() {
        SetLevel(JEventLevel::PhysicsEvent);
        data_in.SetDatabundleName("1");
        data_out.SetShortName("2");
    }
    void Process(const JEvent&) override {
        auto x = data_in().at(0)->x * 10;
        data_out().push_back(new Data {x});
    };
};

struct PEProc : public JEventProcessor {
    Input<Data> data_in {this};
    PEProc() {
        SetLevel(JEventLevel::PhysicsEvent);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_in.SetDatabundleName("2");
    }
    void ProcessSequential(const JEvent&) override {
        (void) (data_in().at(0)->x);
    };
};


TEST_CASE("BasicTopology_Mini") {

    LOG << "Running MinimalPhysicsEventTopology_Benchmarking";

    JApplication app;
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "basic_mini.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");
    //app.SetParameterValue("jana:backoff_interval", "1");
    app.SetParameterValue("jana:backoff_interval", "1");
    //app.SetParameterValue("jana:loglevel", "trace");

    app.Add(new PESrc);
    app.Add(new PEProc);
    app.Add(new JFactoryGeneratorT<PEFac>);

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


TEST_CASE("BasicTopology_JTest") {
    // 5Hz for full reconstruction
    // 20kHz for stripped-down reconstruction (not per-core)
    // Base memory allcation: 100 MB/core + 600MB
    // 1 thread/event, disentangle 1 event, turn into 40.
    // disentangled (single event size) : 12.5 kB / event (before blown up)
    // entangled "block of 40": dis * 40
    
    auto params = new JParameterManager;
    params->SetParameter("jana:loglevel", "off");

    // Log levels get set as soon as JApp gets constructed
    params->SetParameter("jtest:parser:cputime_ms", 2);
    params->SetParameter("jtest:plotter:cputime_ms", 2);

    params->SetParameter("benchmark:resultsdir", "perf_tests");
    params->SetParameter("benchmark:rates_filename", "basic_jtest.dat");
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



} // namespace


