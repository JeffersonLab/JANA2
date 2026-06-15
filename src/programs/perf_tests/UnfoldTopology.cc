
#include "JANA/Utils/JEventLevel.h"
#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/CLI/JBenchmarker.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>


namespace jana::perftest::unfold {

struct Data { size_t x; };

struct BSrc : public JEventSource {

    Output<Data> data_out {this};

    BSrc() {
        SetLevel(JEventLevel::Block);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        data_out.SetShortName("1");
    }
    JEventSource::Result Emit(JEvent& block) override {
        data_out().push_back(new Data {block.GetEventNumber()*3 });
        return Result::Success;
    };
};

struct BFac : public JFactory {
    Input<Data> data_in {this};
    Output<Data> data_out {this};

    BFac() {
        SetLevel(JEventLevel::Block);
        data_in.SetDatabundleName("1");
        data_out.SetShortName("2");
    }
    void Process(const JEvent&) override {
        auto x = data_in().at(0)->x + 7;
        data_out().push_back(new Data {x});
    };
};

struct Unf : public JEventUnfolder {
    Input<Data> parent_data_in {this};
    Output<Data> child_data_out {this};

    Unf() {
        SetParentLevel(JEventLevel::Block);
        SetChildLevel(JEventLevel::PhysicsEvent);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        parent_data_in.SetDatabundleName("2");
        child_data_out.SetShortName("3");
    }
    JEventUnfolder::Result Unfold(const JEvent&, JEvent&, int child_nr) override {
        auto x = parent_data_in().at(0)->x * 10 + child_nr;
        child_data_out().push_back(new Data{x});

        if (child_nr == 9) {
            return Result::NextChildNextParent;
        }
        return Result::NextChildKeepParent;
    };
};

struct PEFac : public JFactory {
    Input<Data> data_in {this};
    Output<Data> data_out {this};
    PEFac() {
        SetLevel(JEventLevel::PhysicsEvent);
        data_in.SetDatabundleName("3");
        data_out.SetShortName("4");
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
        data_in.SetDatabundleName("4");
    }
    void ProcessSequential(const JEvent&) override {
        (void) (data_in().at(0)->x);
    };
};


TEST_CASE("UnfoldToplogy_Mini") {

    LOG << "Running MinimalUnfolderTopology";

    JApplication app;
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


}



