// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/JService.h>
#include <toml.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace jana::services {


class JWiringService : public JService {

public:
    enum class Action { Add, Update, Remove };
    struct Wiring {
        Action action;
        std::string plugin_name;
        std::string type_name;
        std::string prefix;
        JEventLevel level = JEventLevel::None;
        std::vector<std::string> input_names;
        std::vector<JEventLevel> input_levels;
        std::vector<std::vector<std::string>> variadic_input_names;
        std::vector<JEventLevel> variadic_input_levels;
        std::vector<std::string> output_names;
        std::vector<JEventLevel> output_levels;
        std::vector<std::vector<std::string>> variadic_output_names;
        std::map<std::string, std::string> configs;
        bool is_used = false;
    };

    struct WiringSet {
        std::vector<std::string> include_file_names;
        std::vector<std::string> plugin_names;
        std::map<std::string,std::string> shared_parameters;
        std::vector<std::unique_ptr<Wiring>> wirings;
        bool use_short_names=false;
    };

private:
    Parameter<std::string> m_wirings_input_file {this, "jana:wiring_file", "", 
        "Path to TOML file containing wiring definitions"};

    WiringSet m_wiring_set;
    std::map<std::string, Wiring*> m_wirings_from_prefix;
    std::map<std::pair<std::string,std::string>, std::vector<std::string>> m_added_prefixes;
    std::vector<std::string> m_no_added_prefixes; // Because we can't use std::optional yet

public:
    JWiringService();
    void Init() override;

    void AddWirings(std::vector<std::unique_ptr<Wiring>>& wirings, const std::string& source);
    void AddWirings(const toml::table& table, const std::string& source);
    void AddWiringFile(const std::string& filename);

    const std::map<std::string, std::string>& GetSharedParameters() const;
    const std::vector<std::string>& GetPluginNames() const { return m_wiring_set.plugin_names; }
    bool UseShortNames() const { return m_wiring_set.use_short_names; }

    Wiring* GetWiring(const std::string& prefix) const;

    const std::vector<std::string>& GetPrefixesForAddedInstances(const std::string& plugin_name, const std::string& type_name) const;

    const std::vector<std::unique_ptr<Wiring>>& GetAllWirings() const { return m_wiring_set.wirings; }

    void CheckAllWiringsAreUsed();

    static void Overlay(Wiring& above, const Wiring& below);
};

} // namespace jana::services

