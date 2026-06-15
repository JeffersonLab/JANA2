
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JBenchUtils.h>


namespace jana::perftest::source {

struct Data { size_t x; };

struct Src : public JEventSource {

    Parameter<int> latency_us {this, "latency_us", 0};
    Output<Data> data_out {this};

    Src() {
        SetPrefix("sut");
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_out.SetShortName("1");
    }
    JEventSource::Result Emit(JEvent& block) override {
        data_out().push_back(new Data {block.GetEventNumber()*3 });
        JBenchUtils::consume_cpu_us(*latency_us);
        return Result::Success;
    };
};

TEST_CASE("SourceTopology_Mini") {
    LOG << "Running SourceTopology_Mini";
    JApplication app;
    app.SetParameterValue("sut:latency_us", 0); // Infinity Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "source_mini.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");
    app.Add(new Src);
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("SourceTopology_Small") {
    LOG << "Running SourceTopology_Small";
    JApplication app;
    app.SetParameterValue("sut:latency_us", 50); // 20 kHz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "source_small.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");
    app.Add(new Src);
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("SourceTopology_Medium") {
    LOG << "Running SourceTopology_Medium";
    JApplication app;
    app.SetParameterValue("sut:latency_us", 10000); // 100 Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "source_medium.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");
    app.Add(new Src);
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("SourceTopology_Large") {
    LOG << "Running SourceTopology_Large";
    JApplication app;
    app.SetParameterValue("sut:latency_us", 200000); // 5 Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "source_large.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");
    app.Add(new Src);
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

}



