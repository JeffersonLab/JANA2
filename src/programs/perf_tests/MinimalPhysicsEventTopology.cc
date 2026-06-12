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


namespace jana::minimal_physics_event_topology {

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


TEST_CASE("MinimalPhysicsEventTopology") {

    LOG << "Running MinimalPhysicsEventTopology";

    JApplication app;
    app.SetParameterValue("nthreads", "1");
    app.SetParameterValue("jana:nevents", "10000");
    app.SetParameterValue("jana:backoff_interval", "1");
    //app.SetParameterValue("jana:backoff_interval", "10");
    //app.SetParameterValue("jana:loglevel", "trace");

    app.Add(new PESrc);
    app.Add(new PEProc);
    app.Add(new JFactoryGeneratorT<PEFac>);
    app.Run();
}

TEST_CASE("MinimalPhysicsEventTopology_Benchmarking") {

    LOG << "Running MinimalPhysicsEventTopology_Benchmarking";

    JApplication app;
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "minimal_physics_event_topology.dat");
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


} // namespace


