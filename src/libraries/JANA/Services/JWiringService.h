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
    struct Wiring {
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
        std::map<std::string, std::string> configs;
    };

private:
    Parameter<std::string> m_wirings_input_file {this, "jana:wiring_file", "", 
        "Path to TOML file containing wiring definitions"};

    Parameter<bool> m_strict_inheritance {this, "jana:wiring_strictness", true,
        "Allow multiple definitions inside wiring files"};

    std::vector<std::unique_ptr<Wiring>> m_wirings;
    std::map<std::string, Wiring*> m_wirings_from_prefix;
    std::map<std::pair<std::string,std::string>, std::vector<Wiring*>> m_wirings_from_type_and_plugin_names;
    std::vector<Wiring*> m_no_wirings;
    std::map<std::string, std::string> m_shared_parameters;

public:
    JWiringService();
    void Init() override;

    void AddWirings(std::vector<std::unique_ptr<Wiring>>& wirings, const std::string& source);
    void AddWirings(const toml::table& table, const std::string& source);
    void AddWiringFile(const std::string& filename);

    void AddSharedParameters(const toml::table& table, const std::string& source);
    const std::map<std::string, std::string>& GetSharedParameters() const;

    const Wiring*
    GetWiring(const std::string& prefix) const;

    const std::vector<Wiring*>&
    GetWirings(const std::string& plugin_name, const std::string& type_name) const;

    const std::vector<std::unique_ptr<Wiring>>& 
    GetWirings() const { return m_wirings; }

    static void Overlay(Wiring& above, const Wiring& below);
};

} // namespace jana::services

