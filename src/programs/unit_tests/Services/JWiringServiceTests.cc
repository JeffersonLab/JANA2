
#include "JANA/Components/JOmniFactory.h"
#include "JANA/Components/JWiredFactoryGeneratorT.h"
#include "JANA/JException.h"
#include "JANA/Utils/JEventLevel.h"
#include <JANA/Services/JWiringService.h>
#include <catch.hpp>
#include <memory>
#include <toml.hpp>
#include <iostream>


static constexpr std::string_view some_wiring = R"(
    include = ["asdf.toml"]

    [[wiring]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac"
    input_names = ["input_coll_1", "input_coll_2"]
    input_levels = ["Run", "Subrun"]
    output_names = ["output_coll_1", "output_coll_2"]
    
        [wiring.configs]
        x = "22"
        y = "verbose"

    [[wiring]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac_modified"
    input_names = ["input_coll_1", "input_coll_3"]
    output_names = ["output_coll_1_modified", "output_coll_2_modified"]

        [wiring.configs]
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
    [[wiring]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac"

    [[wiring]]
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
    [[wiring]]
    plugin_name = "ECAL"
    type_name = "ClusteringFac"
    prefix = "myfac"

        [wiring.configs]
        x = "22"
        y = "verbose"

    [[wiring]]
    plugin_name = "ECAL"
    type_name = "ClusteringFac"
    prefix = "variantfac"

        [wiring.configs]
        x = "49"
        y = "silent"

    [[wiring]]
    plugin_name = "BCAL"
    type_name = "ClusteringFac"
    prefix = "sillyfac"

        [wiring.configs]
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

struct Cluster { double x,y,E; };

struct WiredOmniFac : jana::components::JOmniFactory<WiredOmniFac> {
    Input<Cluster> m_protoclusters_in {this};
    Output<Cluster> m_clusters_out {this};

    Parameter<int> m_x {this, "x", 1, "x"};
    Parameter<std::string> m_y {this, "y", "silent", "y" };

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        for (auto protocluster : *m_protoclusters_in) {
            m_clusters_out().push_back(new Cluster {protocluster->x, protocluster->y, protocluster->E + 1});
        }
    }
};

static constexpr std::string_view realfacgen_wiring = R"(
    [[wiring]]
    type_name = "WiredOmniFac"
    prefix = "myfac"
    input_names = ["usual_input"]
    output_names = ["usual_output"]

        [wiring.configs]
        x = "22"
        y = "verbose"

    [[wiring]]
    type_name = "WiredOmniFac"
    prefix = "myfac_modified"
    input_names = ["different_input"]
    output_names = ["different_output"]

        [wiring.configs]
        x = "100"
        y = "silent"

)";

TEST_CASE("WiringTests_RealFacGen") {

    JApplication app;

    auto wiring_svc = app.GetService<jana::services::JWiringService>();
    toml::table table = toml::parse(realfacgen_wiring);
    wiring_svc->AddWirings(table, "testcase");

    auto gen = new jana::components::JWiredFactoryGeneratorT<WiredOmniFac>;
    app.Add(gen);
    app.Initialize();

    auto& summary = app.GetComponentSummary();
    jout << summary;
    auto vf = summary.FindComponents("myfac");
    REQUIRE(vf.size() == 1);
    REQUIRE(vf.at(0)->GetOutputs().at(0)->GetName() == "usual_output");

    auto ef = summary.FindComponents("myfac_modified");
    REQUIRE(ef.size() == 1);
    REQUIRE(ef.at(0)->GetOutputs().at(0)->GetName() == "different_output");

    // Check that parameter values propagated from wiring file to parameter manager
    REQUIRE(app.GetParameterValue<int>("myfac:x") == 22);
    REQUIRE(app.GetParameterValue<std::string>("myfac:y") == "verbose");
    REQUIRE(app.GetParameterValue<int>("myfac_modified:x") == 100);
    REQUIRE(app.GetParameterValue<std::string>("myfac_modified:y") == "silent");

    //app.GetJParameterManager()->PrintParameters(2, 0);
    //auto event = std::make_shared<JEvent>(&app);
    //auto facs = event->GetFactorySet()->GetAllMultifactories();

}

struct WiredOmniFacConfig {
    int shared = 0;
    int isolated = 2;
};

struct WiredOmniFacWithShared : jana::components::JOmniFactory<WiredOmniFacWithShared, WiredOmniFacConfig> {
    Input<Cluster> m_protoclusters_in {this};
    Output<Cluster> m_clusters_out {this};

    ParameterRef<int> m_shared {this, "shared", config().shared, "shared", true};
    ParameterRef<int> m_isolated {this, "isolated", config().isolated, "isolated" };

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        for (auto protocluster : *m_protoclusters_in) {
            m_clusters_out().push_back(new Cluster {protocluster->x, protocluster->y, protocluster->E + 1});
        }
    }
};

static constexpr std::string_view sharedparam_wiring = R"(
    [configs]
    shared = "28"

    [[wiring]]
    type_name = "WiredOmniFacWithShared"
    prefix = "myfac"
    input_names = ["usual_input"]
    output_names = ["usual_output"]

        [wiring.configs]
        isolated = "22"

    [[wiring]]
    type_name = "WiredOmniFacWithShared"
    prefix = "myfac_modified"
    input_names = ["different_input"]
    output_names = ["different_output"]

        [wiring.configs]
        isolated = "100"
)";

TEST_CASE("WiringTests_SharedParam") {

    JApplication app;

    auto wiring_svc = app.GetService<jana::services::JWiringService>();
    toml::table table = toml::parse(sharedparam_wiring);
    wiring_svc->AddSharedParameters(table, "testcase");
    wiring_svc->AddWirings(table, "testcase");

    auto gen = new jana::components::JWiredFactoryGeneratorT<WiredOmniFacWithShared>;
    app.Add(gen);
    app.Initialize();

    auto& summary = app.GetComponentSummary();
    //jout << summary;
    auto vf = summary.FindComponents("myfac");
    REQUIRE(vf.size() == 1);
    REQUIRE(vf.at(0)->GetOutputs().at(0)->GetName() == "usual_output");

    auto ef = summary.FindComponents("myfac_modified");
    REQUIRE(ef.size() == 1);
    REQUIRE(ef.at(0)->GetOutputs().at(0)->GetName() == "different_output");

    // Check that parameter values propagated from wiring file to parameter manager
    REQUIRE(app.GetParameterValue<int>("shared") == 28);
    REQUIRE(app.GetParameterValue<int>("myfac:isolated") == 22);
    REQUIRE(app.GetParameterValue<int>("myfac_modified:isolated") == 100);

    //app.GetJParameterManager()->PrintParameters(2, 0);
    //auto event = std::make_shared<JEvent>(&app);
    //auto facs = event->GetFactorySet()->GetAllMultifactories();

}



TEST_CASE("WiringTests_OtherComponents") {

}
