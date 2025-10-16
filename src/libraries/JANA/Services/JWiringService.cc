
#include "JWiringService.h"
#include "toml.hpp"
#include <exception>
#include <memory>
#include <set>

namespace jana::services {

JWiringService::JWiringService() {
    SetPrefix("jana");
}

void JWiringService::Init() {
    LOG_INFO(GetLogger()) << "Initializing JWiringService" << LOG_END;
    // User is _only_ allowed to specify wiring file via parameter
    // This way, we can restrict calling JWiringService::Init until inside JApplication::Init
    // Then we can load the wiring file exactly once. All WiredFactoryGenerators 
    // (recursively) load files

    if (!m_wirings_input_file().empty()) {
        ApplyWiringSet(ParseWiringSetFromFilename(*m_wirings_input_file));
    }
}

void JWiringService::ApplyWiringSet(JWiringService::WiringSet&& wiring_set) {

    // Recursively overlay any included files
    OverlayAllIncludes(wiring_set);

    // Clear everything from before
    m_wiring_set = std::move(wiring_set);
    m_added_prefixes.clear();

    // Populate lookup table
    for (auto& it: m_wiring_set.wirings) {
        if (it.second->action == Action::Add) {
            auto& wiring = it.second;
            m_added_prefixes[{wiring->type_name, wiring->plugin_name}].push_back(wiring->prefix);
        }
    }
}

void JWiringService::OverlayAllIncludes(WiringSet &wiring_set) {
    for (auto& include_file_name : wiring_set.include_file_names) {
        auto include_wiring_set = ParseWiringSetFromFilename(include_file_name);
        OverlayAllIncludes(include_wiring_set);
        OverlayWiringSet(wiring_set, include_wiring_set);
    }
}

std::unique_ptr<JWiringService::Wiring> ParseWiring(const toml::table& f) {

    auto wiring = std::make_unique<JWiringService::Wiring>();

    auto unparsed_action = f["action"].value<std::string>().value_or("(missing)");
    if (unparsed_action == "update") {
        wiring->action = JWiringService::Action::Update;
    }
    else if (unparsed_action == "add") {
        wiring->action = JWiringService::Action::Add;
    }
    else if (unparsed_action == "remove") {
        wiring->action = JWiringService::Action::Remove;
    }
    else {
        throw JException("JWiringService: Invalid action '%s'! Valid values are {'update', 'add', 'remove'}", unparsed_action.c_str());
    }
    wiring->plugin_name = f["plugin_name"].value<std::string>().value_or("");
    wiring->type_name = f["type_name"].value<std::string>().value();
    wiring->prefix = f["prefix"].value<std::string>().value();
    wiring->level = parseEventLevel(f["level"].value_or<std::string>("None"));

    auto input_names = f["input_names"].as_array();
    if (input_names != nullptr) {
        if (wiring->action == JWiringService::Action::Remove) {
            throw JException("Removed wiring has superfluous input names");
        }
        for (const auto& input_name : *input_names) {
            wiring->input_names.push_back(input_name.value<std::string>().value());
        }
    }

    auto variadic_input_names = f["variadic_input_names"].as_array();
    if (variadic_input_names != nullptr) {
        if (wiring->action == JWiringService::Action::Remove) {
            throw JException("Removed wiring has superfluous variadic input names");
        }
        for (const auto& input_name_vec : *variadic_input_names) {
            std::vector<std::string> temp;
            for (const auto& input_name : *(input_name_vec.as_array())) {
                temp.push_back(input_name.as_string()->get());
            }
            wiring->variadic_input_names.push_back(temp);
        }
    }

    auto input_levels = f["input_levels"].as_array();
    if (input_levels != nullptr) {
        if (wiring->action == JWiringService::Action::Remove) {
            throw JException("Removed wiring has superfluous input levels");
        }
        for (const auto& input_level : *input_levels) {
            wiring->input_levels.push_back(parseEventLevel(input_level.value<std::string>().value()));
        }
    }

    auto variadic_input_levels = f["variadic_input_levels"].as_array();
    if (variadic_input_levels != nullptr) {
        if (wiring->action == JWiringService::Action::Remove) {
            throw JException("Removed wiring has superfluous variadic input levels");
        }
        for (const auto& input_level : *variadic_input_levels) {
            wiring->variadic_input_levels.push_back(parseEventLevel(input_level.value<std::string>().value()));
        }
    }

    auto output_names = f["output_names"].as_array();
    if (output_names != nullptr) {
        if (wiring->action == JWiringService::Action::Remove) {
            throw JException("Removed wiring has superfluous output names");
        }
        for (const auto& output_name : *f["output_names"].as_array()) {
            wiring->output_names.push_back(output_name.value<std::string>().value());
        }
    }

    auto variadic_output_names = f["variadic_output_names"].as_array();
    if (variadic_output_names != nullptr) {
        if (wiring->action == JWiringService::Action::Remove) {
            throw JException("Removed wiring has superfluous variadic output names");
        }
        for (const auto& output_name_vec : *variadic_output_names) {
            std::vector<std::string> temp;
            for (const auto& output_name : *(output_name_vec.as_array())) {
                temp.push_back(output_name.as_string()->get());
            }
            wiring->variadic_output_names.push_back(temp);
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

    return wiring;
}


JWiringService::WiringSet JWiringService::ParseWiringSet(const toml::table& table) {

    WiringSet wiring_set;

    // Parse include file names
    auto includes = table["includes"].as_array();
    if (includes != nullptr) {
        for (const auto& include : *includes) {
            wiring_set.include_file_names.push_back(include.as<std::string>()->get());
        }
    }

    // Parse plugin names
    auto plugins = table["plugins"].as_array();
    if (plugins != nullptr) {
        for (const auto& plugin_name : *plugins) {
            wiring_set.include_file_names.push_back(plugin_name.as<std::string>()->get());
        }
    }

    // Parse shared parameters
    auto shared_params = table["configs"].as_table();
    if (shared_params != nullptr) {
        for (const auto& param : *shared_params) {
            std::string key(param.first);
            std::string val = *param.second.value<std::string>();
            wiring_set.shared_parameters[key] = val;
        }
    }

    // Parse use_short_names
    auto use_short_names = table["use_short_names"].as<bool>();
    if (use_short_names != nullptr) {
        wiring_set.use_short_names = use_short_names->get();
    }

    // Parse wirings
    auto wirings_array = table["wiring"].as_array();
    if (wirings_array != nullptr) {
        for (const auto& wiring_node : *wirings_array) {
            auto wiring_table = wiring_node.as_table();
            if (wiring_table != nullptr) {
                auto wiring = ParseWiring(*wiring_table);

                // Check for uniqueness before adding
                auto it = wiring_set.wirings.find(wiring->prefix);
                if (it != wiring_set.wirings.end()) {
                    throw JException("Duplicated prefix '%s' in wiring set", wiring->prefix.c_str());
                }

                // Add to wiring set
                wiring_set.wirings[wiring->prefix] = std::move(wiring);
            }
        }
    }

    return wiring_set;
}

JWiringService::WiringSet JWiringService::ParseWiringSetFromFilename(const std::string& filename) {
    try {
        auto table = toml::parse_file(filename);
        return ParseWiringSet(table);
    }
    catch (const toml::parse_error& err) {
        auto e = JException("Error parsing TOML file: '%s'", filename.c_str());
        e.nested_exception = std::current_exception();
        throw e;
    }
}

JWiringService::Wiring* JWiringService::GetWiring(const std::string& prefix) const {
    auto it = m_wiring_set.wirings.find(prefix);
    if (it == m_wiring_set.wirings.end()) {
        return nullptr;
    }
    return it->second.get();
}

const std::vector<std::string>&
JWiringService::GetPrefixesForAddedInstances(const std::string& plugin_name, const std::string& type_name) const {

    auto it = m_added_prefixes.find({type_name, plugin_name});
    if (it == m_added_prefixes.end()) {
        return m_no_added_prefixes;
    }
    return it->second;
}

void JWiringService::OverlayWiringSet(WiringSet &above, const WiringSet &below) {

    // TODO: Figure out merging of includes, plugins, use_short_names

    // Iterate over 'below' to ensure that every wiring from below propagates above
    for (auto& below_it: below.wirings) {
        auto& below_wiring = below_it.second;
        auto& prefix = below_wiring->prefix;
        const auto& above_it = above.wirings.find(prefix);
        if (above_it == above.wirings.end()) {
            // This is a new wiring, so we add it and we are done
            above.wirings[prefix] = std::make_unique<Wiring>(*below_wiring);
        }
        else {
            // Wiring shows up in both places, so we need to overlay the two
            auto& above_wiring = above_it->second;

            // First we validate that the two wirings are compatible
            if (above_wiring->type_name != below_wiring->type_name) {
                throw JException("Wiring mismatch: type name '%s' vs '%s'",
                                 above_wiring->type_name.c_str(), below_wiring->type_name.c_str());
            }
            if (above_wiring->plugin_name != below_wiring->plugin_name) {
                throw JException("Wiring mismatch: plugin name '%s' vs '%s'",
                                 above_wiring->plugin_name.c_str(), below_wiring->plugin_name.c_str());
            }

            // Next we do the overlay, which modifies above_wiring
            Overlay(*above_wiring, *below_wiring);
        }
    }
}

void JWiringService::Overlay(Wiring& above, const Wiring& below) {

    // In theory this should be handled by the caller, but let's check just in case
    if (above.plugin_name != below.plugin_name) throw JException("Plugin name mismatch!");
    if (above.type_name != below.type_name) throw JException("Type name mismatch!");

    // Overlay actions
    if (above.action == Action::Add) {
        if (below.action != Action::Remove) {
            throw JException("Attempted to add a wiring that is already present. plugin=%s, prefix=%s",
                             above.plugin_name.c_str(), above.prefix.c_str());
        }
        else {
            // Add(Remove(wiring)) = Update(wiring)
            above.action = Action::Update;
        }
    }
    else if (above.action == Action::Update) {
        if (below.action == Action::Add) {
            above.action = Action::Add;
        }
        else if (below.action == Action::Remove) {
            throw JException("Attempted to update a wiring that has been removed. plugin=%s, prefix=%s",
                             above.plugin_name.c_str(), above.prefix.c_str());
        }
        // Update(Update(wiring)) => Update(wiring)
    }
    else if (above.action == Action::Remove) {
        if (below.action == Action::Remove) {
            throw JException("Attempted to remove a wiring that has already been removed. plugin=%s, prefix=%s",
                             above.plugin_name.c_str(), above.prefix.c_str());
        }
        // Remove(Add(wiring)) => Remove(wiring)  // This one is odd, might reconsider
        // Remove(Update(wiring)) => Remove(wiring)
    }

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

const std::map<std::string, std::string>& JWiringService::GetSharedParameters() const {
    return m_wiring_set.shared_parameters;
}

void JWiringService::CheckAllWiringsAreUsed() {
    std::vector<JWiringService::Wiring*> m_unused_wirings;
    for (const auto& wiring : m_wiring_set.wirings) {
        if (wiring.second->is_used == false) {
            m_unused_wirings.push_back(wiring.second.get());
        }
    }
    if (m_unused_wirings.size() != 0) {
        LOG_ERROR(GetLogger()) << "Wirings were found but never used:";
        for (auto wiring : m_unused_wirings) {
            LOG_ERROR(GetLogger()) << "  " << wiring->type_name << " " << wiring->prefix;
        }
        throw JException("Unused wirings found");
    }
}

} // namespace jana::services
