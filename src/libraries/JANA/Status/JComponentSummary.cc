
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iostream>
#include <iomanip>
#include <JANA/Utils/JTablePrinter.h>
#include "JComponentSummary.h"

std::ostream& operator<<(std::ostream& os, JComponentSummary const& cs) {

    os << "Component Summary" << std::endl << std::endl;

    JTablePrinter table;
    table.AddColumn("Plugin");
    table.AddColumn("Type");
    table.AddColumn("Level");
    table.AddColumn("Name");
    table.AddColumn("Tag");

    for (const auto& source : cs.event_sources) {
        table | source.plugin_name | "JEventSource" | source.level | source.type_name | source.source_name;
    }
    for (const auto& unfolder : cs.event_unfolders) {
        table | unfolder.plugin_name | "JEventUnfolder" | unfolder.level | unfolder.type_name | unfolder.prefix;
    }

    for (const auto& proc : cs.event_processors) {
        table | proc.plugin_name | "JEventProcessor" | proc.level | proc.type_name | proc.prefix;
    }

    os << table << std::endl;

    JTablePrinter factoryTable;
    factoryTable.AddColumn("Plugin");
    factoryTable.AddColumn("Factory");
    factoryTable.AddColumn("Level");
    factoryTable.AddColumn("Collection type");
    factoryTable.AddColumn("Collection tag");
    for (const auto& factory : cs.factories) {
        factoryTable | factory.plugin_name | factory.factory_name | factory.level | factory.object_name | factory.factory_tag;
    }
    os << factoryTable << std::endl;
    return os;

}
