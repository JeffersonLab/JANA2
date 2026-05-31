
#include "JANA/Utils/JEventLevel.h"
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>


namespace jana::minimal_source_topology {

struct Data { size_t x; };

struct Src : public JEventSource {

    Output<Data> data_out {this};

    Src() {
        //SetLevel(JEventLevel::Block);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_out.SetShortName("1");
    }
    JEventSource::Result Emit(JEvent& block) override {
        data_out().push_back(new Data {block.GetEventNumber()*3 });
        return Result::Success;
    };
};

TEST_CASE("Mini") {
    LOG << "Running Mini";
    JApplication app;
    //app.SetParameterValue("jana:loglevel", "debug");
    app.SetParameterValue("jana:nevents", "4000000");
    app.SetParameterValue("nthreads", "16");
    app.Add(new Src);
    app.Run();
}

TEST_CASE("MinimalSourceTopology") {
    LOG << "Running MinimalSourceTopology";
    JApplication app;
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "minimal_source_topology.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");
    app.Add(new Src);
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


}



