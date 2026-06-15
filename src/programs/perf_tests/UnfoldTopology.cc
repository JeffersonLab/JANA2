
#include "JANA/Utils/JEventLevel.h"
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JBenchUtils.h>


namespace jana::perftest::unfold {

struct Data { size_t x; };

struct BSrc : public JEventSource {

    Output<Data> data_out {this};
    Parameter<int> latency_us {this, "latency_us", 0};

    BSrc() {
        SetPrefix("bsrc");
        SetLevel(JEventLevel::Block);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_out.SetShortName("1");
    }
    JEventSource::Result Emit(JEvent& block) override {
        data_out().push_back(new Data {block.GetEventNumber()*3 });
        JBenchUtils::consume_cpu_us(*latency_us);
        return Result::Success;
    };
};

struct BFac : public JFactory {
    Input<Data> data_in {this};
    Output<Data> data_out {this};
    Parameter<int> latency_us {this, "latency_us", 0};

    BFac() {
        SetPrefix("bfac");
        SetLevel(JEventLevel::Block);
        data_in.SetDatabundleName("1");
        data_out.SetShortName("2");
    }
    void Process(const JEvent&) override {
        auto x = data_in().at(0)->x + 7;
        data_out().push_back(new Data {x});
        JBenchUtils::consume_cpu_us(*latency_us);
    };
};

struct Unf : public JEventUnfolder {
    Input<Data> parent_data_in {this};
    Output<Data> child_data_out {this};
    Parameter<int> latency_us {this, "latency_us", 0};

    Unf() {
        SetPrefix("unf");
        SetParentLevel(JEventLevel::Block);
        SetChildLevel(JEventLevel::PhysicsEvent);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        parent_data_in.SetDatabundleName("2");
        child_data_out.SetShortName("3");
    }
    JEventUnfolder::Result Unfold(const JEvent&, JEvent&, int child_nr) override {
        auto x = parent_data_in().at(0)->x * 10 + child_nr;
        child_data_out().push_back(new Data{x});
        JBenchUtils::consume_cpu_us(*latency_us);

        if (child_nr == 9) {
            return Result::NextChildNextParent;
        }
        return Result::NextChildKeepParent;
    };
};

struct PEFac : public JFactory {
    Input<Data> data_in {this};
    Output<Data> data_out {this};
    Parameter<int> latency_us {this, "latency_us", 0};
    PEFac() {
        SetPrefix("pefac");
        SetLevel(JEventLevel::PhysicsEvent);
        data_in.SetDatabundleName("3");
        data_out.SetShortName("4");
    }
    void Process(const JEvent&) override {
        auto x = data_in().at(0)->x * 10;
        data_out().push_back(new Data {x});
        JBenchUtils::consume_cpu_us(*latency_us);
    };
};

struct PEProc : public JEventProcessor {
    Input<Data> data_in {this};
    Parameter<int> latency_us {this, "latency_us", 0};
    PEProc() {
        SetPrefix("peproc");
        SetLevel(JEventLevel::PhysicsEvent);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_in.SetDatabundleName("4");
    }
    void ProcessSequential(const JEvent&) override {
        (void) (data_in().at(0)->x);
        JBenchUtils::consume_cpu_us(*latency_us);
    };
};


TEST_CASE("UnfoldTopology_Mini") {

    LOG << "Running UnfoldTopology_Mini";

    JApplication app;
    app.SetParameterValue("bsrc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("bfac:latency_us", 0); // Infinity Hz
    app.SetParameterValue("unf:latency_us", 0); // Infinity Hz
    app.SetParameterValue("pefac:latency_us", 0); // Infinity Hz
    app.SetParameterValue("peproc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "unfold_mini.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    app.Add(new BSrc);
    app.Add(new Unf);
    app.Add(new PEProc);
    app.Add(new JFactoryGeneratorT<BFac>);
    app.Add(new JFactoryGeneratorT<PEFac>);

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("UnfoldTopology_Small") {
    LOG << "Running UnfoldTopology_Small";

    JApplication app;
    app.SetParameterValue("bsrc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("bfac:latency_us", 1'000'000 / 500); // 500 Hz
    app.SetParameterValue("unf:latency_us", 0); // Infinity Hz
    app.SetParameterValue("pefac:latency_us", 1'000'000 / 5000); // 5 kHz
    app.SetParameterValue("peproc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "unfold_small.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    app.Add(new BSrc);
    app.Add(new Unf);
    app.Add(new PEProc);
    app.Add(new JFactoryGeneratorT<BFac>);
    app.Add(new JFactoryGeneratorT<PEFac>);

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("UnfoldTopology_Medium") {
    LOG << "Running UnfoldTopology_Medium";

    JApplication app;
    app.SetParameterValue("bsrc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("bfac:latency_us", 1'000'000 / 10); // 10 Hz
    app.SetParameterValue("unf:latency_us", 0); // Infinity Hz
    app.SetParameterValue("pefac:latency_us", 1'000'000 / 100); // 100 Hz
    app.SetParameterValue("peproc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "unfold_medium.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    app.Add(new BSrc);
    app.Add(new Unf);
    app.Add(new PEProc);
    app.Add(new JFactoryGeneratorT<BFac>);
    app.Add(new JFactoryGeneratorT<PEFac>);

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("UnfoldTopology_Large") {
    LOG << "Running UnfoldTopology_Large";

    JApplication app;
    app.SetParameterValue("bsrc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("bfac:latency_us", 1'000'000*2); // 0.5 Hz
    app.SetParameterValue("unf:latency_us", 0); // Infinity Hz
    app.SetParameterValue("pefac:latency_us", 1'000'000 / 5); // 5 Hz
    app.SetParameterValue("peproc:latency_us", 0); // Infinity Hz
    app.SetParameterValue("benchmark:resultsdir", "perf_tests");
    app.SetParameterValue("benchmark:rates_filename", "unfold_large.dat");
    app.SetParameterValue("benchmark:use_log_scale", true);
    app.SetParameterValue("benchmark:minthreads", "1");
    app.SetParameterValue("benchmark:maxthreads", "32");

    app.Add(new BSrc);
    app.Add(new Unf);
    app.Add(new PEProc);
    app.Add(new JFactoryGeneratorT<BFac>);
    app.Add(new JFactoryGeneratorT<PEFac>);

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


}



