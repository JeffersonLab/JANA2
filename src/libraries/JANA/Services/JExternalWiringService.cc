
#include "JExternalWiringService.h"
#include "JANA/Utils/JEventLevel.h"
#include <memory>

/*
    std::string plugin_name;
    std::string type_name;
    std::string prefix;
    JEventLevel level = JEventLevel::None;
    std::vector<std::string> input_names;
    std::vector<JEventLevel> input_levels;
    std::vector<std::string> output_names;
    std::map<std::string, std::string> configs;
*/

std::vector<std::unique_ptr<JExternalWiringService::Wiring>> JExternalWiringService::parse_table(const toml::table& table) {

    std::vector<std::unique_ptr<Wiring>> wirings;

    auto facs = table["factory"].as_array();
    for (const auto& fac : *facs) {
        auto wiring = std::make_unique<Wiring>();
        auto& f = *fac.as_table();
        wiring->prefix = f["prefix"].value<std::string>().value();
        wiring->type_name = f["type_name"].value<std::string>().value();
        wiring->plugin_name = f["plugin_name"].value<std::string>().value();
        //wiring->level = parseEventLevel(f["level"].value<std::string>().value());
        wirings.push_back(std::move(wiring));
    }
    return wirings;
}
