
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <string>
#include <vector>
#include <JANA/Utils/JEventLevel.h>

struct JFactorySummary {
    JEventLevel level;
    std::string plugin_name;
    std::string factory_name;
    std::string factory_tag;
    std::string object_name;
};

struct JEventSourceSummary {
    JEventLevel level;
    std::string plugin_name;
    std::string type_name;
    std::string source_name;
};

struct JEventProcessorSummary {
    JEventLevel level;
    std::string plugin_name;
    std::string type_name;
    std::string prefix;
};

struct JEventUnfolderSummary {
    JEventLevel level;
    std::string plugin_name;
    std::string type_name;
    std::string prefix;
};

struct JComponentSummary {
    std::vector<JFactorySummary> factories;
    std::vector<JEventSourceSummary> event_sources;
    std::vector<JEventProcessorSummary> event_processors;
    std::vector<JEventUnfolderSummary> event_unfolders;
};

std::ostream& operator<<(std::ostream& os, JComponentSummary const & cs);


