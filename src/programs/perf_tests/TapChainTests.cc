
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>


namespace jana::tap_chain_tests {

struct Data { size_t x; };

struct Src : public JEventSource {

    Output<Data> data_out {this};

    Src() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_out.SetShortName("1");
    }
    JEventSource::Result Emit(JEvent& evt) override {
        data_out().push_back(new Data {evt.GetEventNumber()*3 });
        return Result::Success;
    };
};

struct Fac : public JFactory {
    Input<Data> data_in {this};
    Output<Data> data_out {this};

    Fac() {
        data_in.SetDatabundleName("1");
        data_out.SetShortName("2");
    }
    void Process(const JEvent&) override {
        auto x = data_in().at(0)->x + 7;
        data_out().push_back(new Data {x});
    };
};

struct Proc : public JEventProcessor {
    Input<Data> data_in {this};
    Proc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_in.SetDatabundleName("2");
    }
    void ProcessSequential(const JEvent&) override {
        (void) (data_in().at(0)->x);
    };
};


TEST_CASE("TapChainTests_1") {

    LOG << "Running TapChainTests_1";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    app.Add(new Proc);

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTests_4") {

    LOG << "Running TapChainTests_4";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<4; ++i) {
        app.Add(new Proc);
    }

    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}

TEST_CASE("TapChainTests_16") {

    LOG << "Running TapChainTests_16";

    JApplication app;
    app.Add(new Src);
    app.Add(new JFactoryGeneratorT<Fac>);
    for (int i=0; i<16; ++i) {
        app.Add(new Proc);
    }
    JBenchmarker benchmarker(&app);
    benchmarker.RunUntilFinished();
}


}



