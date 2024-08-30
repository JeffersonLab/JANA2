
#include "JANA/JException.h"
#include "JANA/Utils/JEventLevel.h"
#include <JANA/Services/JExternalWiringService.h>
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

TEST_CASE("ExternalWiringTests") {

    jana::services::JExternalWiringService sut;
    toml::table table = toml::parse(some_wiring);
    auto wirings = sut.parse_table(table);
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

TEST_CASE("ExternalWiringTests_DuplicatePrefixes") {

    jana::services::JExternalWiringService sut;
    toml::table table = toml::parse(duplicate_prefixes);
    try {
        auto wirings = sut.parse_table(table);
        REQUIRE(1 == 0);
    }
    catch (const JException& e) {
        std::cout << e << std::endl;
    }
}

TEST_CASE("ExternalWiringTests_Overlay") {
    using Wiring = jana::services::JExternalWiringService::Wiring;
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

    auto sut = jana::services::JExternalWiringService();
    auto result = sut.overlay(std::move(above), std::move(below));
    REQUIRE(result->input_names[2] == "make");
    REQUIRE(result->input_levels[1] == JEventLevel::PhysicsEvent);
    REQUIRE(result->configs["x"] == "6.18");
    REQUIRE(result->configs["y"] == "42");
}


