
#include <JANA/Services/JExternalWiringService.h>
#include <catch.hpp>
#include <toml.hpp>


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

    JExternalWiringService sut;
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