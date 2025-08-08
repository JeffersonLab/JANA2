
#include <catch.hpp>
#include <JANA/JVersion.h>
#include <JANA/JApplicationFwd.h>
#include <JANA/Components/JOmniFactory.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>

struct MyHit {int e; };

#if JANA2_HAVE_PODIO

#include <PodioDatamodel/ExampleHitCollection.h>

struct MyFac : public JOmniFactory<MyFac> {
    Input<MyHit> hits_in {this};
    VariadicPodioInput<ExampleHit> variadic_podio_hits_in {this};
    PodioInput<ExampleHit> podio_hits_in {this};

    Output<MyHit> hits_out {this};
    VariadicPodioOutput<ExampleHit> variadic_podio_hits_out {this};
    PodioOutput<ExampleHit> podio_hits_out {this};

    MyFac() {
        hits_out.SetNotOwnerFlag(true);
    }
    void Configure() { }
    void ChangeRun(int32_t /*run_nr*/) { }
    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        REQUIRE(hits_out().size() == 0);
        REQUIRE(podio_hits_out()->size() == 0);
        REQUIRE(variadic_podio_hits_out().size() == 2);
        REQUIRE(variadic_podio_hits_out().at(0)->size() == 0);
        REQUIRE(variadic_podio_hits_out().at(1)->size() == 0);

        for (auto hit : *hits_in) {
            hits_out().push_back(const_cast<MyHit*>(hit));
        }

        podio_hits_out()->setSubsetCollection();
        variadic_podio_hits_out().at(0)->setSubsetCollection();

        for (auto hit : *podio_hits_in) {
            podio_hits_out()->push_back(hit);
            variadic_podio_hits_out().at(0)->push_back(hit);
        }

        variadic_podio_hits_out().at(1)->push_back(MutableExampleHit(22, 1.1, 1.1, 1.1, 10, 0));
    }
};

void test_single_event(JEvent& event) {
    event.Insert(new MyHit{99}, "lw");
    ExampleHitCollection coll;
    coll.push_back(MutableExampleHit{14,0.0,0.0,0.0,100,0});
    coll.push_back(MutableExampleHit{21,0.0,0.0,0.0,100,0});
    event.InsertCollection<ExampleHit>(std::move(coll), "podio");

    ExampleHitCollection coll2;
    coll2.push_back(MutableExampleHit{30,0.0,0.0,0.0,100,0});
    event.InsertCollection<ExampleHit>(std::move(coll2), "v_podio_0");

    ExampleHitCollection coll3;
    coll3.push_back(MutableExampleHit{10101,0.0,0.0,0.0,100,0});
    event.InsertCollection<ExampleHit>(std::move(coll3), "v_podio_1");

    auto hits = event.Get<MyHit>("lw2");
    REQUIRE(hits.size() == 1);
    REQUIRE(hits.at(0)->e == 99);

    auto podio_hits = event.GetCollection<ExampleHit>("podio2");
    REQUIRE(podio_hits->size() == 2);
    REQUIRE(podio_hits->at(0).cellID() == 14);
    REQUIRE(podio_hits->at(1).cellID() == 21);

    auto podio_hits_v2 = event.GetCollection<ExampleHit>("v_podio_2");
    REQUIRE(podio_hits_v2 ->size() == 2);
    REQUIRE(podio_hits_v2->at(0).cellID() == 14);
    REQUIRE(podio_hits_v2->at(1).cellID() == 21);

    auto podio_hits_v3 = event.GetCollection<ExampleHit>("v_podio_3");
    REQUIRE(podio_hits_v3 ->size() == 1);
    REQUIRE(podio_hits_v3->at(0).cellID() == 22);
}

TEST_CASE("JOmniFactoryTests_VariadicWiring") {
    JApplication app;
    app.Add(new JOmniFactoryGeneratorT<MyFac>(
        "sut", 
        {"lw", "v_podio_0", "v_podio_1", "podio"},
        {"lw2", "v_podio_2", "v_podio_3", "podio2"}));

    auto event = std::make_shared<JEvent>(&app);
    test_single_event(*event);
    event->Clear();
    test_single_event(*event);
}

#endif


struct MyFac2 : public JOmniFactory<MyFac2> {
    Output<MyHit> hits_out {this};

    void Configure() { }
    void ChangeRun(int32_t /*run_nr*/) { }
    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {
        REQUIRE(hits_out().size() == 0);
        hits_out().push_back(new MyHit{22});
    }
};


TEST_CASE("JOmniFactoryTests_OutputsCleared") {
    JApplication app;
    app.Add(new JOmniFactoryGeneratorT<MyFac2>(
        "sut", 
        {},
        {"huegelgrab"}));

    auto event = std::make_shared<JEvent>(&app);
    REQUIRE(event->GetSingleStrict<MyHit>("huegelgrab")->e == 22);

    event->Clear();
    REQUIRE(event->GetSingleStrict<MyHit>("huegelgrab")->e == 22);
}

struct MyFac3 : public JOmniFactory<MyFac3> {
    Input<MyHit> hits_in {this};
    Output<MyHit> hits_out {this};

    void Configure() { }
    void ChangeRun(int32_t /*run_nr*/) { }
    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {
        REQUIRE(hits_in().at(0)->e == 123);
        hits_out().push_back(new MyHit{1234});
    }
};


TEST_CASE("JOmniFactoryTests_LightweightInputTag") {
    JApplication app;
    app.Add(new JOmniFactoryGeneratorT<MyFac3>(
        "sut", 
        {"huegelgrab"},
        {"schlafen"}));

    auto event = std::make_shared<JEvent>(&app);
    event->Insert<MyHit>(new MyHit{123}, "huegelgrab");
    REQUIRE(event->GetSingleStrict<MyHit>("schlafen")->e == 1234);

}
