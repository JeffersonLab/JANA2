
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
    output_names = ["output_coll_1", "output_coll_2"]
    
        [factory.configs]
        x = 22
        y = "verbose"

    [[factory]]
    plugin_name = "BCAL"
    type_name = "MyFac"
    prefix = "myfac_modified"
    input_names = ["input_coll_1", "input_coll_3"]
    output_names = ["output_coll_1_modified", "output_coll_2_modified"]

        [factory.configs]
        x = 100 
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

}
