
#include "JWiringService.h"
#include "JANA/Utils/JEventLevel.h"
#include <memory>
#include <ostream>

namespace jana::services {

void JWiringService::Init() {
    LOG_INFO(GetLogger()) << "Initializing JWiringService" << LOG_END;
    // User is _only_ allowed to specify wiring file via parameter
    // This way, we can restrict calling JWiringService::Init until inside JApplication::Init
    // Then we can load the wiring file exactly once. All WiredFactoryGenerators 
    // (Recursively) load files

}

std::unique_ptr<JWiringService::Wiring> JWiringService::overlay(std::unique_ptr<Wiring>&& above, std::unique_ptr<Wiring>&& below) {

    // In theory this should be handled by the caller, but let's check just in case
    if (above->plugin_name != below->plugin_name) throw JException("Plugin name mismatch!");
    if (above->type_name != below->type_name) throw JException("Type name mismatch!");

    if (above->input_names.empty() && !below->input_names.empty()) {
        above->input_names = std::move(below->input_names);
    }
    if (above->input_levels.empty() && !below->input_levels.empty()) {
        above->input_levels = std::move(below->input_levels);
    }
    if (above->output_names.empty() && !below->output_names.empty()) {
        above->output_names = std::move(below->output_names);
    }
    for (const auto& [key, val] : below->configs) {
        if (above->configs.find(key) == above->configs.end()) {
            above->configs[key] = val;
        }
    }
    // below gets automatically deleted
    return std::move(above);
}


std::vector<std::unique_ptr<JWiringService::Wiring>> JWiringService::parse_table(const toml::table& table) {

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




} // namespace jana::services
