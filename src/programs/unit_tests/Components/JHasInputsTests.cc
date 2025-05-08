#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JObject.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

#include <PodioDatamodel/ExampleHitCollection.h>

namespace jana2::components::hasinputstests {

struct TestHit : public JObject {
    int cell_row;
    int cell_col;
    double energy;

    TestHit(int cell_row, int cell_col, double energy) : cell_row(cell_row), cell_col(cell_col), energy(energy) {}
};

struct TestEventSource : public JEventSource {

    TestEventSource() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    Result Emit(JEvent& event) override {

        std::vector<TestHit*> det_a_hits;
        det_a_hits.push_back(new TestHit(5, 4, 100.5));
        det_a_hits.push_back(new TestHit(5, 5, 99.8));
        det_a_hits.push_back(new TestHit(6, 4, 70.1));
        event.Insert(det_a_hits, "detector_a_hits");

        std::vector<TestHit*> det_b_hits;
        det_b_hits.push_back(new TestHit(1, 0, 50.5));
        det_b_hits.push_back(new TestHit(2, 0, 65.1));
        det_b_hits.push_back(new TestHit(3, 0, 22.8));
        event.Insert(det_b_hits, "detector_b_hits");

        ExampleHitCollection det_c_hits;
        det_c_hits.push_back(MutableExampleHit(14, 0.0, 1.1, 2.2, 52.2, 0));
        event.InsertCollection<ExampleHit>(std::move(det_c_hits), "detector_c_hits");

        ExampleHitCollection det_d_hits;
        det_d_hits.push_back(MutableExampleHit(0, 0.0, 0.0, 0.0, 1000.1, 0));
        det_d_hits.push_back(MutableExampleHit(0, 0.0, 0.0, 0.0, 500.5, 1));
        det_d_hits.push_back(MutableExampleHit(0, 0.0, 0.0, 0.0, 300.1, 2));
        event.InsertCollection<ExampleHit>(std::move(det_d_hits), "detector_d_hits");

        ExampleHitCollection det_e_hits;
        det_e_hits.push_back(MutableExampleHit(4, 10.0, 10.0, 10.0, 77.0, 3));
        event.InsertCollection<ExampleHit>(std::move(det_e_hits), "detector_e_hits");

        return Result::Success;
    }
};

struct TestProc : public JEventProcessor {

    Input<TestHit> m_det_a_hits_in {this};

    //VariadicInput<TestHit> m_det_a_hits_in {this};

    PodioInput<ExampleHit> m_det_c_hits_in {this};

    VariadicPodioInput<ExampleHit> m_det_de_hits_in {this};

    TestProc() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
        m_det_a_hits_in.Configure({.name = "detector_a_hits"});
        m_det_c_hits_in.Configure({.name = "detector_c_hits"});
        m_det_de_hits_in.ConfigureVariadic({.names = {"detector_d_hits", "detector_e_hits"}});
    }

    void Process(const JEvent&) override {
        REQUIRE(m_det_a_hits_in->size() == 3);
        REQUIRE(m_det_a_hits_in->at(2)->cell_col == 4);

        REQUIRE(m_det_c_hits_in->size() == 1);
        REQUIRE(m_det_c_hits_in->at(0).energy() == 52.2);

        REQUIRE(m_det_de_hits_in().at(0)->size() == 3);
        REQUIRE(m_det_de_hits_in().at(0)->at(0).energy() == 1000.1);

        REQUIRE(m_det_de_hits_in().at(1)->size() == 1);
        REQUIRE(m_det_de_hits_in().at(1)->at(0).energy() == 77.0);
    }
};

TEST_CASE("JHasInputs_BasicTests") {
    JApplication app;
    app.Add(new TestEventSource);
    app.Add(new TestProc);
    app.SetParameterValue("jana:nevents", 1);
    app.Run();
};

}
