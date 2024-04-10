#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/JEventProcessor.h>
#include <JANA/Omni/JOmniFactory.h>
#include <JANA/Omni/JOmniFactoryGeneratorT.h>

namespace jana {

template <typename OutputCollectionT, typename MultifactoryT>
MultifactoryT* RetrieveMultifactory(JFactorySet* facset, std::string output_collection_name) {
    auto fac = facset->GetFactory<OutputCollectionT>(output_collection_name);
    REQUIRE(fac != nullptr);
    auto helper = dynamic_cast<JMultifactoryHelper<OutputCollectionT>*>(fac);
    REQUIRE(helper != nullptr);
    auto multifactory = helper->GetMultifactory();
    REQUIRE(multifactory != nullptr);
    auto typed = dynamic_cast<MultifactoryT*>(multifactory);
    REQUIRE(typed != nullptr);
    return typed;
}

namespace component_unfolder_param_tests {


struct TestUnfolder : public JEventUnfolder {

    Parameter<float> threshold {this, "threshold", 16.0, "The max cutoff threshold [V * A * kg^-1 * m^-2 * sec^-3]"};
    Parameter<int> bucket_count {this, "bucket_count", 22, "The total number of buckets [dimensionless]"};

    TestUnfolder() {
        SetPrefix("my_unfolder");
    }
};


TEST_CASE("JEventUnfolderParametersTests") {
    JApplication app;
    auto* sut = new TestUnfolder;
    app.Add(sut);

    SECTION("JEventUnfolder using default parameters") {
        app.Initialize();
        sut->DoInit();
        REQUIRE(sut->threshold() == 16.0);
        REQUIRE(sut->bucket_count() == 22);
    }
    SECTION("JEventUnfolder using overridden parameters") {
        app.SetParameterValue("my_unfolder:threshold", 12.0);
        app.Initialize();
        sut->DoInit();
        REQUIRE(sut->threshold() == 12.0);
        REQUIRE(sut->bucket_count() == 22);
    }
}
} // component_unfolder_param_tests





namespace component_processor_param_tests {

struct TestProc : public JEventProcessor {

    Parameter<float> threshold {this, "threshold", 16.0, "The max cutoff threshold [V * A * kg^-1 * m^-2 * sec^-3]"};
    Parameter<int> bucket_count {this, "bucket_count", 22, "The total number of buckets [dimensionless]"};

    TestProc() {
        SetPrefix("my_proc");
    }
};


TEST_CASE("JEventProcessorParametersTests") {
    JApplication app;
    auto* sut = new TestProc;
    app.Add(sut);

    SECTION("JEventProcessor using default parameters") {
        app.Initialize();
        sut->DoInitialize();
        REQUIRE(sut->threshold() == 16.0);
        REQUIRE(sut->bucket_count() == 22);
    }
    SECTION("JEventProcessor using overridden parameters") {
        app.SetParameterValue("my_proc:threshold", 12.0);
        app.Initialize();
        sut->DoInitialize();
        REQUIRE(sut->threshold() == 12.0);
        REQUIRE(sut->bucket_count() == 22);
    }

}

} // namsepace component_processor_param_tests


namespace component_omnifactory_param_tests {

struct MyCluster {
    int x;
};

struct TestFac : public JOmniFactory<TestFac> {

    Output<MyCluster> clusters_out {this, "clusters_out"};

    Parameter<float> threshold {this, "threshold", 16.0, "The max cutoff threshold [V * A * kg^-1 * m^-2 * sec^-3]"};
    Parameter<int> bucket_count {this, "bucket_count", 22, "The total number of buckets [dimensionless]"};

    TestFac() {
    }

    void Configure() {
    }
    
    void ChangeRun(int32_t) final {
    }


    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void Execute(int64_t, uint64_t) {
    }
};


TEST_CASE("JOmniFactoryParametersTests") {
    JApplication app;


    SECTION("JOmniFactory using default parameters") {
        app.Initialize();
        JOmniFactoryGeneratorT<TestFac> facgen (&app);
        facgen.AddWiring("ECalTestAlg", {}, {"specific_clusters_out"});
        JFactorySet facset;
        facgen.GenerateFactories(&facset);
        auto sut = RetrieveMultifactory<MyCluster,TestFac>(&facset, "specific_clusters_out");
        // RetrieveMultifactory() will call DoInitialize() for us
        
        REQUIRE(sut->threshold() == 16.0);
        REQUIRE(sut->bucket_count() == 22);
    }

    /*
     * FIXME: In EICrecon first though

    SECTION("JOmniFactory using facgen parameters") {
        app.Initialize();
        JOmniFactoryGeneratorT<TestFac> facgen (&app);
        facgen.AddWiring("my_fac", {}, {"specific_clusters_out"}, {{"bucket_count","444"}});
        JFactorySet facset;
        facgen.GenerateFactories(&facset);
        auto sut = RetrieveMultifactory<MyCluster,TestFac>(&facset, "specific_clusters_out");
        // RetrieveMultifactory() will call DoInitialize() for us

        REQUIRE(sut->threshold() == 16.0);
        REQUIRE(sut->bucket_count() == 444);
    }

    SECTION("JOmniFactory using overridden parameters") {
        app.SetParameterValue("my_fac:threshold", 12.0);
        app.Initialize();

        JOmniFactoryGeneratorT<TestFac> facgen (&app);
        facgen.AddWiring("my_fac", {}, {"specific_clusters_out"}, {{"threshold", "55.5"}});
        JFactorySet facset;
        facgen.GenerateFactories(&facset);
        auto sut = RetrieveMultifactory<MyCluster,TestFac>(&facset, "specific_clusters_out");

        REQUIRE(sut->threshold() == 12.0);
        REQUIRE(sut->bucket_count() == 22);
    }

    */

} // TEST_CASE

} // namespace component_omnifactory_param_tests
} // namespace jana
