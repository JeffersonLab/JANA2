
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
    sourcesTable.AddColumn("Plugin");
    sourcesTable.AddColumn("Name");
    sourcesTable.AddColumn("Source");
    sourcesTable.AddColumn("Level");
    for (const auto& source : cs.event_sources) {
        sourcesTable | source.plugin_name | source.type_name | source.source_name | source.level;
    }
    sourcesTable.Render(os);
    os << "  PROCESSORS" << std::endl;
    JTablePrinter procsTable;
    procsTable.AddColumn("Plugin");
    procsTable.AddColumn("Name");
    procsTable.AddColumn("Level");
    for (const auto& proc : cs.event_processors) {
        procsTable | proc.plugin_name | proc.type_name | proc.level;
    }
    procsTable.Render(os);

    if (cs.event_unfolders.size() != 0) {
        os << "  UNFOLDERS" << std::endl;
        JTablePrinter unfoldersTable;
        unfoldersTable.AddColumn("Plugin");
        unfoldersTable.AddColumn("Name");
        unfoldersTable.AddColumn("Level");
        for (const auto& unfolder : cs.event_unfolders) {
            unfoldersTable | unfolder.plugin_name | unfolder.type_name | unfolder.level;
        }
        sourcesTable.Render(os);
    }

    os << "  FACTORIES" << std::endl;

    JTablePrinter factoryTable;
    factoryTable.AddColumn("Plugin");
    factoryTable.AddColumn("Object name");
    factoryTable.AddColumn("Tag");
    factoryTable.AddColumn("Level");
    for (const auto& factory : cs.factories) {
        factoryTable | factory.plugin_name | factory.object_name | factory.factory_tag | factory.level;
    }
    factoryTable.Render(os);
    return os;

}
