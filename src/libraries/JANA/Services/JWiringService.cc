
#include "JWiringService.h"
#include "toml.hpp"
#include <exception>
#include <memory>
#include <set>

namespace jana::services {

void JWiringService::Init() {
    LOG_INFO(GetLogger()) << "Initializing JWiringService" << LOG_END;
    // User is _only_ allowed to specify wiring file via parameter
    // This way, we can restrict calling JWiringService::Init until inside JApplication::Init
    // Then we can load the wiring file exactly once. All WiredFactoryGenerators 
    // (recursively) load files

    if (!m_wirings_input_file().empty()) {
        AddWiringFile(*m_wirings_input_file);
    }
}

void JWiringService::AddWirings(std::vector<std::unique_ptr<Wiring>>& wirings_bundle, const std::string& bundle_source) {

    std::set<std::string> prefixes_in_bundle;
    for (auto& wiring: wirings_bundle) {

        // Assert that this wiring's prefix is unique _within_ this bundle.
        auto bundle_it = prefixes_in_bundle.find(wiring->prefix);
        if (bundle_it != prefixes_in_bundle.end()) {
            throw JException("Duplicated prefix '%s' in wiring bundle '%s'", wiring->prefix.c_str(), bundle_source.c_str());
        }
        prefixes_in_bundle.insert(wiring->prefix);

        // Check whether we have seen this prefix before
        auto it = m_wirings_from_prefix.find(wiring->prefix);
        if (it == m_wirings_from_prefix.end()) {
            // This is a new wiring
            m_wirings_from_prefix[wiring->prefix] = wiring.get();
            m_wirings_from_type_and_plugin_names[{wiring->type_name, wiring->plugin_name}].push_back(wiring.get());
            m_wirings.push_back(std::move(wiring));
        }
        else {
            // Wiring is already defined; overlay this wiring _below_ the existing wiring
            // First we do some sanity checks
            if (wiring->type_name != it->second->type_name) {
                throw JException("Wiring mismatch: type name '%s' vs '%s'", wiring->type_name.c_str(), it->second->type_name.c_str());
            }
            if (wiring->plugin_name != it->second->plugin_name) {
                throw JException("Wiring mismatch: plugin name '%s' vs '%s'", wiring->plugin_name.c_str(), it->second->plugin_name.c_str());
            }
            Overlay(*(it->second), *wiring);
            // Useful information from `wiring` has been copied into `it->second`.
            // `wiring` will now be automatically destroyed
        }
    }
    // At this point all wirings have been moved out of wirings_bundle, so we clear it to avoid confusing callers
    wirings_bundle.clear();
}

void JWiringService::AddWirings(const toml::table& table, const std::string& source) {

    std::vector<std::unique_ptr<Wiring>> wirings;
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

        wiring->plugin_name = f["plugin_name"].value<std::string>().value_or("");
        wiring->type_name = f["type_name"].value<std::string>().value();
        wiring->prefix = f["prefix"].value<std::string>().value();

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
    AddWirings(wirings, source);
}

void JWiringService::AddWiringFile(const std::string& filename) {
    try {
        auto tbl = toml::parse_file(filename);
        AddSharedParameters(tbl, filename);
        AddWirings(tbl, filename);
    }
    catch (const toml::parse_error& err) {
        auto e = JException("Error parsing TOML file: '%s'", filename.c_str());
        e.nested_exception = std::current_exception();
        throw e;
    }
}

const JWiringService::Wiring* JWiringService::GetWiring(const std::string& prefix) const {
    auto it = m_wirings_from_prefix.find(prefix);
    if (it == m_wirings_from_prefix.end()) {
        return nullptr;
    }
    return it->second;
}

const std::vector<JWiringService::Wiring*>&
JWiringService::GetWirings(const std::string& plugin_name, const std::string& type_name) const {

    auto it = m_wirings_from_type_and_plugin_names.find({type_name, plugin_name});
    if (it == m_wirings_from_type_and_plugin_names.end()) {
        return m_no_wirings;
    }
    return it->second;
}


void JWiringService::Overlay(Wiring& above, const Wiring& below) {

    // In theory this should be handled by the caller, but let's check just in case
    if (above.plugin_name != below.plugin_name) throw JException("Plugin name mismatch!");
    if (above.type_name != below.type_name) throw JException("Type name mismatch!");

    if (above.input_names.empty() && !below.input_names.empty()) {
        above.input_names = std::move(below.input_names);
    }
    if (above.input_levels.empty() && !below.input_levels.empty()) {
        above.input_levels = std::move(below.input_levels);
    }
    if (above.output_names.empty() && !below.output_names.empty()) {
        above.output_names = std::move(below.output_names);
    }
    for (const auto& [key, val] : below.configs) {
        if (above.configs.find(key) == above.configs.end()) {
            above.configs[key] = val;
        }
    }
}


void JWiringService::AddSharedParameters(const toml::table& table, const std::string& /*source*/) {
    auto shared_params = table["configs"].as_table();
    if (shared_params == nullptr) {
        LOG_INFO(GetLogger()) << "No configs found!" << LOG_END;
        return;
    }
    for (const auto& param : *shared_params) {
        std::string key(param.first);
        std::string val = *param.second.value<std::string>();
        m_shared_parameters[key] = val;
    }
}



const std::map<std::string, std::string>& JWiringService::GetSharedParameters() const {
    return m_shared_parameters;
}


} // namespace jana::services
