
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iostream>
#include <JANA/Utils/JTablePrinter.h>
#include "JComponentSummary.h"



void JComponentSummary::Add(JComponentSummary::Component* component) {

    m_components.push_back(component);
    m_component_lookups[component->GetTypeName()].push_back(component);
    m_component_lookups[component->GetPrefix()].push_back(component);

    size_t output_count = component->m_outputs.size();
    for (size_t i=0; i<output_count; ++i) {
        JComponentSummary::Collection* coll = component->m_outputs[i];
        auto lookups = m_collection_lookups[coll->GetName()]; // Auto-create if missing
        // See if we already have this collection
        bool found = false;
        for (JComponentSummary::Collection* existing_coll : lookups) {
            if (*existing_coll == *coll) {
                component->m_outputs[i] = existing_coll;
                existing_coll->m_producers.push_back(component);
                delete coll;
                found = true;
            }
        }
        if (!found) {
            coll->AddProducer(component);
            lookups.push_back(coll);
            m_collections.push_back(coll);
            m_collection_lookups[coll->GetTypeName()].push_back(coll);
        }
    }
    size_t input_count = component->m_inputs.size();
    for (size_t i=0; i<input_count; ++i) {
        JComponentSummary::Collection* coll = component->m_inputs[i];
        auto lookups = m_collection_lookups[coll->GetName()]; // Auto-create if missing
        // See if we already have this collection
        bool found = false;
        for (JComponentSummary::Collection* existing_coll : lookups) {
            if (*existing_coll == *coll) {
                component->m_inputs[i] = existing_coll;
                existing_coll->m_consumers.push_back(component);
                delete coll;
                found = true;
            }
        }
        if (!found) {
            coll->AddConsumer(component);
            lookups.push_back(coll);
            m_collections.push_back(coll);
            m_collection_lookups[coll->GetTypeName()].push_back(coll);
        }
    }

    for (JComponentSummary::Collection* output : component->m_outputs) {
        JFactorySummary fac;
        fac.level = output->GetLevel();
        fac.plugin_name = component->GetPluginName();
        fac.factory_name = component->GetTypeName();
        fac.factory_tag = output->GetTag().empty() ? output->GetName() : output->GetTag();
        fac.object_name = output->GetTypeName();
        factories.push_back(std::move(fac));
    }
}


void PrintCollectionTable(std::ostream& os, const JComponentSummary& cs) {

    JTablePrinter coll_table;
    coll_table.AddColumn("Type");
    coll_table.AddColumn("Name");
    coll_table.AddColumn("Tag");
    coll_table.AddColumn("Level");
    /*
    coll_table.AddColumn("Direction");
    coll_table.AddColumn("Factory Type");
    coll_table.AddColumn("Factory Prefix");
    coll_table.AddColumn("Plugin");
    */

    os << "  Collection Summary" << std::endl;

    for (const auto* coll : cs.GetAllCollections()) {
        coll_table | coll->GetTypeName() | coll->GetName() | coll->GetTag() | coll->GetLevel();
        /*
        bool first = true;

        for (const auto* producer : coll->GetProducers()) {
            if (!first) {
                coll_table | "" | "" | "" | "";
            }
            coll_table | "Output" | producer->GetTypeName() | producer->GetPrefix() | producer->GetPluginName();
            first = false;
        }
        for (const auto* consumer : coll->GetConsumers()) {
            if (!first) {
                coll_table | "" | "" | "" | "";
            }
            coll_table | "Input" | consumer->GetTypeName() | consumer->GetPrefix() | consumer->GetPluginName();
            first = false;
        }
        */
    }
    os << coll_table;
}

void PrintComponentTable(std::ostream& os, const JComponentSummary& cs) {


    JTablePrinter comp_table;
    comp_table.AddColumn("Base");
    comp_table.AddColumn("Type");
    comp_table.AddColumn("Prefix");
    comp_table.AddColumn("Level");
    comp_table.AddColumn("Plugin");

    os << "  Component Summary" << std::endl;

    for (const auto* comp : cs.GetAllComponents()) {
        comp_table | comp->GetBaseName() | comp->GetTypeName() | comp->GetPrefix() | comp->GetLevel() | comp->GetPluginName();
    }
    os << comp_table;
}


std::ostream& operator<<(std::ostream& os, JComponentSummary const& cs) {

    PrintComponentTable(os,cs);
    return os;
}


std::ostream& operator<<(std::ostream& os, const JComponentSummary::Collection& col) {

    os << "  Type:  " << col.GetTypeName() << std::endl;
    os << "  Name:  " << col.GetName() << std::endl;
    os << "  Tag:   " << col.GetTag() << std::endl;
    os << "  Level: " << col.GetLevel() << std::endl;
    os << std::endl;

    if (!col.GetProducers().empty()) {
        os << "  Producers:" << std::endl;
        JTablePrinter table;
        table.AddColumn("Type");
        table.AddColumn("Prefix");
        table.AddColumn("Level");
        table.AddColumn("Plugin");
        for (const auto* p : col.GetProducers()) {
            table | p->GetTypeName() | p->GetPrefix() | p->GetLevel() | p->GetPluginName();
        }
        os << table.Render();
    }
    os << std::endl;

    if (!col.GetConsumers().empty()) {
        os << "  Consumers:" << std::endl;
        JTablePrinter table;
        table.AddColumn("Type");
        table.AddColumn("Prefix");
        table.AddColumn("Level");
        table.AddColumn("Plugin");
        for (const auto& c : col.GetConsumers()) {
            table | c->GetTypeName() | c->GetPrefix() | c->GetLevel() | c->GetPluginName();
        }
        os << table.Render();
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, const JComponentSummary::Component& c) {

    os << "  Type:   " << c.GetTypeName() << std::endl;
    os << "  Prefix: " << c.GetPrefix() << std::endl;
    os << "  Level:  " << c.GetLevel() << std::endl;
    os << "  Plugin: " << c.GetPluginName() << std::endl;

    if (!c.GetInputs().empty()) {
        os << std::endl;
        os << "  Inputs:" << std::endl;
        JTablePrinter table;
        table.AddColumn("Type");
        table.AddColumn("Name");
        table.AddColumn("Tag");
        table.AddColumn("Level");
        for (const auto* col : c.GetInputs()) {
            table | col->GetTypeName() | col->GetName() | col->GetTag() | col->GetLevel();
        }
        os << table.Render();
    }

    if (!c.GetOutputs().empty()) {
        os << std::endl;
        os << "  Outputs:" << std::endl;
        JTablePrinter table;
        table.AddColumn("Type");
        table.AddColumn("Name");
        table.AddColumn("Tag");
        table.AddColumn("Level");
        for (const auto* col : c.GetOutputs()) {
            table | col->GetTypeName() | col->GetName() | col->GetTag() | col->GetLevel();
        }
        os << table.Render();
    }
    return os;
}



