
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JCOMPONENTSUMMARY_H
#define JANA2_JCOMPONENTSUMMARY_H

#include <string>
#include <vector>

struct JFactorySummary {
    std::string plugin_name;
    std::string factory_name;
    std::string factory_tag;
    std::string object_name;
};

struct JEventSourceSummary {
    std::string plugin_name;
    std::string type_name;
    std::string source_name;
};

struct JEventProcessorSummary {
    std::string plugin_name;
    std::string type_name;
};

struct JComponentSummary {
    std::vector<JFactorySummary> factories;
    std::vector<JEventSourceSummary> event_sources;
    std::vector<JEventProcessorSummary> event_processors;
};

std::ostream& operator<<(std::ostream& os, JComponentSummary const & cs);



#endif //JANA2_JCOMPONENTSUMMARY_H
