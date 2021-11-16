
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iostream>
#include <iomanip>
#include <JANA/Utils/JTablePrinter.h>
#include "JComponentSummary.h"

std::ostream& operator<<(std::ostream& os, JComponentSummary const& cs) {

    os << "Component Summary" << std::endl << std::endl;
    os << std::left;
    os << "  SOURCES" << std::endl;
    JTablePrinter sourcesTable;
    sourcesTable.addColumn("Plugin");
    sourcesTable.addColumn("Name");
    sourcesTable.addColumn("Source");
    for (const auto& source : cs.event_sources) {
	sourcesTable | source.plugin_name | source.type_name | source.source_name;
    }
    sourcesTable.render(os);
    os << "  PROCESSORS" << std::endl;
    JTablePrinter procsTable;
    procsTable.addColumn("Plugin");
    procsTable.addColumn("Name");
    for (const auto& proc : cs.event_processors) {
	procsTable | proc.plugin_name | proc.type_name;
    }
    procsTable.render(os);

    os << "  FACTORIES" << std::endl;

    JTablePrinter factoryTable;
    factoryTable.addColumn("Plugin");
    factoryTable.addColumn("Object name");
    factoryTable.addColumn("Tag");
    for (const auto& factory : cs.factories) {
        factoryTable | factory.plugin_name | factory.object_name | factory.factory_tag;
    }
    factoryTable.render(os);
    return os;

}
