
#include "JANA/Components/JOmniFactory.h"
#include "JANA/Components/JOmniFactoryGeneratorT.h"
#include <PodioDatamodel/ExampleClusterCollection.h>
#include "JANA/JException.h"
#include "JANA/Utils/JEventLevel.h"
#include <JANA/Services/JWiringService.h>
#include <catch.hpp>
#include <memory>
#include <toml.hpp>
#include <iostream>


static constexpr std::string_view some_wiring = R"(
    include = ["asdf.toml"]

    [[factory]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac"
    input_names = ["input_coll_1", "input_coll_2"]
    input_levels = ["Run", "Subrun"]
    output_names = ["output_coll_1", "output_coll_2"]
    
        [factory.configs]
        x = "22"
        y = "verbose"

    [[factory]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac_modified"
    input_names = ["input_coll_1", "input_coll_3"]
    output_names = ["output_coll_1_modified", "output_coll_2_modified"]

        [factory.configs]
        x = "100"
        y = "verbose"

)";

TEST_CASE("WiringTests") {

    jana::services::JWiringService sut;
    toml::table table = toml::parse(some_wiring);
    sut.AddWirings(table, "testcase");

    const auto& wirings = sut.GetWirings();
    REQUIRE(wirings.size() == 2);
    REQUIRE(wirings[0]->prefix == "myfac");
    REQUIRE(wirings[1]->prefix == "myfac_modified");
    REQUIRE(wirings[0]->plugin_name == "BCAL");
    REQUIRE(wirings[0]->input_names[1] == "input_coll_2");
    REQUIRE(wirings[1]->input_names[1] == "input_coll_3");
    REQUIRE(wirings[0]->input_levels.size() == 2);
    REQUIRE(wirings[0]->input_levels[0] == JEventLevel::Run);
    REQUIRE(wirings[0]->input_levels[1] == JEventLevel::Subrun);
    REQUIRE(wirings[1]->input_levels.size() == 0);
    REQUIRE(wirings[0]->configs.size() == 2);
    REQUIRE(wirings[0]->configs["x"] == "22");
    REQUIRE(wirings[0]->configs["y"] == "verbose");

}

static constexpr std::string_view duplicate_prefixes = R"(
    [[factory]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac"

    [[factory]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac"
)";

TEST_CASE("WiringTests_DuplicatePrefixes") {

    jana::services::JWiringService sut;
    toml::table table = toml::parse(duplicate_prefixes);
    try {
        sut.AddWirings(table,"testcase");
        REQUIRE(1 == 0);
    }
    catch (const JException& e) {
        std::cout << e << std::endl;
    }
}

TEST_CASE("WiringTests_Overlay") {
    using Wiring = jana::services::JWiringService::Wiring;
    auto above = std::make_unique<Wiring>();
    above->prefix = "myfac";
    above->type_name = "ClusteringFac";
    above->plugin_name = "FCAL";
    above->input_names = {"this", "should", "make", "it"};
    above->configs["x"] = "6.18";

    auto below = std::make_unique<Wiring>();
    below->prefix = "myfac";
    below->type_name = "ClusteringFac";
    below->plugin_name = "FCAL";
    below->input_names = {"this", "should", "NOT", "make", "it"};
    below->input_levels = {JEventLevel::Run, JEventLevel::PhysicsEvent};
    below->configs["x"] = "7.6";
    below->configs["y"] = "42";

    auto sut = jana::services::JWiringService();
    sut.Overlay(*above, *below);
    REQUIRE(above->input_names[2] == "make");
    REQUIRE(above->input_levels[1] == JEventLevel::PhysicsEvent);
    REQUIRE(above->configs["x"] == "6.18");
    REQUIRE(above->configs["y"] == "42");
}

static constexpr std::string_view fake_wiring_file = R"(
    [[factory]]
    plugin_name = "ECAL"
    type_name = "ClusteringFac"
    prefix = "myfac"

        [factory.configs]
        x = "22"
        y = "verbose"

    [[factory]]
    plugin_name = "ECAL"
    type_name = "ClusteringFac"
    prefix = "variantfac"

        [factory.configs]
        x = "49"
        y = "silent"

    [[factory]]
    plugin_name = "BCAL"
    type_name = "ClusteringFac"
    prefix = "sillyfac"

        [factory.configs]
        x = "618"
        y = "mediocre"
)";


TEST_CASE("WiringTests_FakeFacGen") {
    jana::services::JWiringService sut;
    toml::table table = toml::parse(fake_wiring_file);
    sut.AddWirings(table, "testcase");

    using Wiring = jana::services::JWiringService::Wiring;
    std::vector<std::unique_ptr<Wiring>> fake_facgen_wirings;
    
    // One gets overlaid with an existing wiring
    auto a = std::make_unique<Wiring>();
    a->plugin_name = "ECAL";
    a->type_name = "ClusteringFac";
    a->prefix = "variantfac";
    a->configs["x"] = "42";
    fake_facgen_wirings.push_back(std::move(a));
    
    // The other is brand new
    auto b = std::make_unique<Wiring>();
    b->plugin_name = "ECAL";
    b->type_name = "ClusteringFac";
    b->prefix = "exuberantfac";
    b->configs["x"] = "27";
    fake_facgen_wirings.push_back(std::move(b));

    // We should end up with three in total
    sut.AddWirings(fake_facgen_wirings, "fake_facgen");
    auto final_wirings = sut.GetWirings("ECAL", "ClusteringFac");

    REQUIRE(final_wirings.size() == 3);

    REQUIRE(final_wirings[0]->prefix == "myfac");
    REQUIRE(final_wirings[1]->prefix == "variantfac");
    REQUIRE(final_wirings[2]->prefix == "exuberantfac");

    REQUIRE(final_wirings[0]->configs["x"] == "22"); // from file only
    REQUIRE(final_wirings[1]->configs["x"] == "49"); // file overrides facgen
    REQUIRE(final_wirings[2]->configs["x"] == "27"); // from facgen only

}

struct WiredOmniFac : jana::components::JOmniFactory<WiredOmniFac> {
    PodioInput<ExampleCluster> m_protoclusters_in {this};
    PodioOutput<ExampleCluster> m_clusters_out {this};

    Parameter<int> m_x {this, "x", 1, "x"};
    Parameter<std::string> m_y {this, "y", "silent", "y" };

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        auto cs = std::make_unique<ExampleClusterCollection>();
        for (auto protocluster : *m_protoclusters_in()) {
            auto cluster = cs->create();
            cluster.energy((m_x() * protocluster.energy()) + 1);
        }
        m_clusters_out() = std::move(cs);
    }
};

TEST_CASE("WiringTests_RealFacGen") {

    JApplication app;

    auto wiring_svc = app.GetService<jana::services::JWiringService>();
    toml::table table = toml::parse(fake_wiring_file);
    wiring_svc->AddWirings(table, "testcase");

    auto gen = new jana::components::JOmniFactoryGeneratorT<WiredOmniFac>;

    // One gets overlaid with an existing wiring
    gen->AddWiring("variantfac", {"protoclusters"}, {"clusters"}, {{"x","42"}});

    // The other is brand new
    gen->AddWiring("exuberantfac", {"protoclusters"}, {"wackyclusters"}, {{"x","27"}});

    // We should end up with three in total

    app.Add(gen);
    app.Initialize();

    auto& summary = app.GetComponentSummary();
    LOG << summary;
    auto vf = summary.FindComponents("variantfac");
    REQUIRE(vf.size() == 1);
    REQUIRE(vf.at(0)->GetOutputs().at(0)->GetName() == "clusters");

    auto ef = summary.FindComponents("exuberantfac");
    REQUIRE(ef.size() == 1);
    REQUIRE(ef.at(0)->GetOutputs().at(0)->GetName() == "wackyclusters");

    auto event = std::make_shared<JEvent>(&app);
    auto facs = event->GetFactorySet()->GetAllMultifactories();

}


