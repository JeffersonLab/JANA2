// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once
#include <JANA/Utils/JEventLevel.h>
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
    std::map<std::string, std::unique_ptr<Wiring>> m_wirings;

public:
    void parse_file(std::string filename);
    std::vector<const Wiring*> get_wirings(std::string plugin_name, std::string type_name);
    const Wiring* get_wiring(std::string plugin_name, std::string type_name, std::string prefix);

};
