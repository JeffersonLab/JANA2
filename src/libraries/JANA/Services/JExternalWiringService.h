// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/Utils/JEventLevel.h>
#include <toml.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>


class JExternalWiringService {

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

private:
    std::vector<std::unique_ptr<Wiring>> m_wirings;
    //std::set<std::string> m_prefixes;
    //std::map<std::pair<std::string,std::string>, std::map<std::string,const Wiring*>> m_wirings_index;
    // { (plugin_name,typename) : {prefix : const Wiring*}}

public:
    void parse_file(std::string filename);
    void add_wiring(std::unique_ptr<Wiring> wiring);
    const Wiring* get_wiring(std::string plugin_name, std::string type_name, std::string prefix);
    const std::vector<std::unique_ptr<Wiring>>& get_wirings();
    std::vector<std::unique_ptr<Wiring>> parse_table(const toml::table& table);

};
