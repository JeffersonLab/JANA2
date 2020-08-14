
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iostream>
#include <iomanip>
#include "JComponentSummary.h"

std::ostream& operator<<(std::ostream& os, JComponentSummary const& cs) {

    os << "Component Summary" << std::endl << std::endl;
    os << std::left;
    os << "  SOURCES" << std::endl;
    os << "  +--------------------------+-------------------------------+-------------------------------+" << std::endl;
    os << "  | Plugin                   | Name                          | Source                        |" << std::endl;
    os << "  +--------------------------+-------------------------------+-------------------------------+" << std::endl;
    for (const auto& source : cs.event_sources) {
        os << "  | " << std::setw(25) << source.plugin_name
           << "| " << std::setw(30) << source.type_name
           << "| " << std::setw(30) << source.source_name
           << "|" << std::endl;
    }
    os << "  +--------------------------+-------------------------------+-------------------------------+" << std::endl;
    os << "  PROCESSORS" << std::endl;
    os << "  +--------------------------+---------------------------------------------------------------+" << std::endl;
    os << "  | Plugin                   | Name                                                          |" << std::endl;
    os << "  +--------------------------+---------------------------------------------------------------+" << std::endl;
    for (const auto& proc : cs.event_processors) {
        os << "  | " << std::setw(25) << proc.plugin_name
           << "| " << std::setw(62) << proc.type_name
           << "|" << std::endl;
    }
    os << "  +--------------------------+---------------------------------------------------------------+" << std::endl;
    os << "  FACTORIES" << std::endl;
    os << "  +--------------------------+-------------------------------+-------------------------------+" << std::endl;
    os << "  | Plugin                   | Object name                   | Tag                           |" << std::endl;
    os << "  +--------------------------+-------------------------------+-------------------------------+" << std::endl;
    for (const auto& factory : cs.factories) {
        os << "  | " << std::setw(25) << factory.plugin_name
           << "| " << std::setw(30) << factory.object_name
           << "| " << std::setw(30) << factory.factory_tag
           << "|" << std::endl;
    }
    os << "  +--------------------------+-------------------------------+-------------------------------+" << std::endl;
    return os;

}
