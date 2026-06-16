
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JBenchUtils.h>


namespace jana::perftest::tapchain {

struct Data { size_t x; };

struct Src : public JEventSource {

    Output<Data> data_out {this};
    Parameter<int> latency_us {this, "latency_us", 0};

    Src() {
        SetPrefix("src");
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_out.SetShortName("1");
    }
    JEventSource::Result Emit(JEvent& evt) override {
        data_out().push_back(new Data {evt.GetEventNumber()*3 });
        JBenchUtils::consume_cpu_us(*latency_us);
        return Result::Success;
    };
};

struct Fac : public JFactory {
    Input<Data> data_in {this};
    Output<Data> data_out {this};
    Parameter<int> latency_us {this, "latency_us", 0};

    Fac() {
        SetPrefix("fac");
        data_in.SetDatabundleName("1");
        data_out.SetShortName("2");
    }
    void Process(const JEvent&) override {
        auto x = data_in().at(0)->x + 7;
        data_out().push_back(new Data {x});
        JBenchUtils::consume_cpu_us(*latency_us);
    };
};

struct Proc : public JEventProcessor {
    Input<Data> data_in {this};
    Parameter<int> latency_us {this, "latency_us", 0};
    Proc() {
        SetPrefix("proc");
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_in.SetDatabundleName("2");
    }
    void ProcessSequential(const JEvent&) override {
        (void) (data_in().at(0)->x);
        JBenchUtils::consume_cpu_us(*latency_us);
    };
};


TEST_CASE("TapChainTopology_1_Mini") {

    LOG << "Running TapChainTopology_1_Mini";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    app.Add(new Proc);

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 0); // Inf Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_1_mini.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_4_Mini") {

    LOG << "Running TapChainTopology_4_Mini";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<4; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_4_mini.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_16_Mini") {

    LOG << "Running TapChainTopology_16_Mini";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<16; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 0); // Inf Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_16_mini.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_1_Small") {

    LOG << "Running TapChainTopology_1_Small";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    app.Add(new Proc);

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 5000); // 5 kHz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_1_small.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_4_Small") {

    LOG << "Running TapChainTopology_4_Small";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<4; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 5000); // 5 kHz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_4_small.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_16_Small") {

    LOG << "Running TapChainTopology_16_Small";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<16; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 5000); // 5 kHz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_16_small.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


TEST_CASE("TapChainTopology_1_Medium") {

    LOG << "Running TapChainTopology_1_Medium";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    app.Add(new Proc);

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 100); // 100 Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_1_medium.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_4_Medium") {

    LOG << "Running TapChainTopology_4_Medium";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<4; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 100); // 100 Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_4_medium.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_16_Medium") {

    LOG << "Running TapChainTopology_16_Medium";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<16; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 100); // 100 Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_16_medium.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_1_Large") {

    LOG << "Running TapChainTopology_1_Large";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    app.Add(new Proc);

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 5); // 5 Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_1_large.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_4_Large") {

    LOG << "Running TapChainTopology_4_Large";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<4; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 5); // 5 Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_4_large.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_16_Large") {

    LOG << "Running TapChainTopology_16_Large";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<16; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 0); // Inf Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 5); // 5 Hz
    app.SetParameterValue("proc:latency_us", 0); // Inf Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_16_large.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTopology_Pipelining") {

    LOG << "Running TapChainTopology_Pipelining";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<4; ++i) {
        app.Add(new Proc);
    }

    app.SetParameterValue("src:latency_us", 1'000'000 / 100); // 100 Hz
    app.SetParameterValue("fac:latency_us", 1'000'000 / 100); // 100 Hz
    app.SetParameterValue("proc:latency_us", 1'000'000 / 100); // 100 Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "tapchain_pipelining.dat");
    app.SetParameterValue("benchmark:use_log_scale", false);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "16");

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


}



