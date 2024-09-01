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

namespace jana {
namespace services {


class JWiringService : public JService {

public:
    struct Wiring {
        std::string plugin_name;
        std::string type_name;
        std::string prefix;
        JEventLevel level = JEventLevel::None;
        std::vector<std::string> input_names;
        std::vector<JEventLevel> input_levels;
        std::vector<std::string> output_names;
        std::map<std::string, std::string> configs;
    };

    using WiringIndex = std::map<std::pair<std::string,std::string>, std::map<std::string,Wiring*>>;
    // { (plugin_name,typename) : {prefix : const Wiring*}}

private:
    Parameter<std::string> m_wirings_input_file {this, "jana:wiring_file", "wiring.toml", 
        "Path to TOML file containing wiring definitions"};

    Parameter<bool> m_strict_inheritance {this, "jana:wiring_strictness", true,
        "Allow multiple definitions inside wiring files"};

    std::vector<std::unique_ptr<Wiring>> m_wirings;
    WiringIndex m_wirings_index;

public:
    void Init() override;
    void parse_file(std::string filename);
    void add_wiring(std::unique_ptr<Wiring> wiring);
    std::unique_ptr<Wiring> overlay(std::unique_ptr<Wiring>&& above, std::unique_ptr<Wiring>&& below);
    const Wiring* get_wiring(std::string plugin_name, std::string type_name, std::string prefix);
    const std::vector<std::unique_ptr<Wiring>>& get_wirings();
    std::vector<std::unique_ptr<Wiring>> parse_table(const toml::table& table);

};

} // namespace services
} // namespace jana
