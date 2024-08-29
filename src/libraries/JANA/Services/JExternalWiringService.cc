
#include "JExternalWiringService.h"
#include "JANA/Utils/JEventLevel.h"
#include <memory>
#include <ostream>


std::vector<std::unique_ptr<JExternalWiringService::Wiring>> JExternalWiringService::parse_table(const toml::table& table) {

    std::vector<std::unique_ptr<Wiring>> wirings;
    std::map<std::string, const Wiring*> prefix_lookup;

    auto facs = table["factory"].as_array();
    if (facs == nullptr) {
        throw JException("No factories found!");
    }
    for (const auto& fac : *facs) {
        auto wiring = std::make_unique<Wiring>();

        if (fac.as_table() == nullptr) {
            throw JException("Invalid format: 'factory' is not a table");
        }
        auto& f = *fac.as_table();

        wiring->plugin_name = f["plugin_name"].value<std::string>().value();
        wiring->type_name = f["type_name"].value<std::string>().value();
        wiring->prefix = f["prefix"].value<std::string>().value();
        auto it = prefix_lookup.find(wiring->prefix);
        if (it != prefix_lookup.end()) {
            std::ostringstream oss;
            oss << "Duplicated factory prefix in wiring file: " << std::endl;
            oss << "    Prefix:      " << wiring->prefix << std::endl;
            oss << "    Type name:   " << wiring->type_name << " vs " << it->second->type_name << std::endl;
            oss << "    Plugin name: " << wiring->plugin_name << " vs " << it->second->plugin_name << std::endl;
            throw JException(oss.str());
        }
        prefix_lookup[wiring->prefix] = wiring.get();

        wiring->level = parseEventLevel(f["level"].value_or<std::string>("None"));

        auto input_names = f["input_names"].as_array();
        if (input_names != nullptr) {
            for (const auto& input_name : *f["input_names"].as_array()) {
                wiring->input_names.push_back(input_name.value<std::string>().value());
            }
        }

        auto output_names = f["output_names"].as_array();
        if (output_names != nullptr) {
            for (const auto& output_name : *f["output_names"].as_array()) {
                wiring->output_names.push_back(output_name.value<std::string>().value());
            }
        }

        auto input_levels = f["input_levels"].as_array();
        if (input_levels != nullptr) {
            for (const auto& input_level : *input_levels) {
                wiring->input_levels.push_back(parseEventLevel(input_level.value<std::string>().value()));
            }
        }

        auto configs = f["configs"].as_table();
        if (configs != nullptr) {
            for (const auto& config : *configs) {
                std::string config_name(config.first);
                // For now, parse all config values as strings. 
                // Later we may go for a deeper integration with toml types and/or with JParameterManager.
                wiring->configs[config_name] = config.second.value<std::string>().value();
            }
        }

        wirings.push_back(std::move(wiring));
    }
    return wirings;
}


