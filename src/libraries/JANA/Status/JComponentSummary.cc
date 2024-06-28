
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iostream>
#include <iomanip>
#include <JANA/Utils/JTablePrinter.h>
#include "JComponentSummary.h"



void JComponentSummary::Add(JComponentSummary::Component* component) {
    m_components.push_back(component);
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
        }
    }

    for (JComponentSummary::Collection* output : component->m_outputs) {
        JComponentSummary::Factory fac;
        fac.level = output->GetLevel();
        fac.plugin_name = component->GetPluginName();
        fac.factory_name = component->GetTypeName();
        fac.factory_tag = output->GetTag().empty() ? output->GetName() : output->GetTag();
        fac.object_name = output->GetTypeName();
        factories.push_back(std::move(fac));
    }
}


std::ostream& operator<<(std::ostream& os, JComponentSummary const& cs) {

    os << "Component Summary" << std::endl << std::endl;

    JTablePrinter comp_table;
    comp_table.AddColumn("Plugin");
    comp_table.AddColumn("Type");
    comp_table.AddColumn("Level");
    comp_table.AddColumn("Name");
    comp_table.AddColumn("Tag");


    JTablePrinter coll_table;
    coll_table.AddColumn("Plugin");
    coll_table.AddColumn("Factory");
    coll_table.AddColumn("Level");
    coll_table.AddColumn("Collection type");
    coll_table.AddColumn("Collection tag");

    std::string component_type;
    for (const auto* comp : cs.GetAllComponents()) {
        comp_table | comp->GetPluginName();
        switch(comp->GetComponentType()) {
            case JComponentSummary::ComponentType::Source: comp_table | "Source"; break;
            case JComponentSummary::ComponentType::Processor: comp_table | "Processor"; break;
            case JComponentSummary::ComponentType::Factory: comp_table | "Factory"; break;
            case JComponentSummary::ComponentType::Unfolder: comp_table | "Unfolder"; break;
            case JComponentSummary::ComponentType::Folder: comp_table | "Folder"; break;
        }
        comp_table | comp->GetLevel() | comp->GetTypeName() | comp->GetPrefix();
        
        bool first = true;
        for (auto* coll : comp->GetOutputs()) {
            if (first) {
                coll_table | comp->GetPluginName() | comp->GetTypeName() | coll->GetLevel() | coll->GetTypeName() | coll->GetName();
            }
            else {
                coll_table | "" | "" | coll->GetLevel() | coll->GetTypeName() | coll->GetName();
            }
        }
    }

    os << comp_table << std::endl;
    os << coll_table << std::endl;
    return os;
}


std::ostream& operator<<(std::ostream& os, const JComponentSummary::Collection& col) {

    os << "    Name:  " << col.GetName() << std::endl;
    os << "    Type:  " << col.GetTypeName() << std::endl;
    os << "    Tag:   " << col.GetTag() << std::endl;
    os << "    Level: " << col.GetLevel() << std::endl;
    os << std::endl;

    if (!col.GetProducers().empty()) {
        os << "Produced by:" << std::endl;
        JTablePrinter table;
        table.AddColumn("Prefix");
        table.AddColumn("Type");
        table.AddColumn("Level");
        table.AddColumn("Plugin");
        for (const auto* p : col.GetProducers()) {
            table | p->GetPrefix() | p->GetTypeName() | p->GetLevel() | p->GetPluginName();
        }
        os << table.Render();
    }
    os << std::endl;

    if (!col.GetConsumers().empty()) {
        os << "Consumed by:" << std::endl;
        JTablePrinter table;
        table.AddColumn("Prefix");
        table.AddColumn("Type");
        table.AddColumn("Level");
        table.AddColumn("Plugin");
        for (const auto& c : col.GetConsumers()) {
            table | c->GetPrefix() | c->GetTypeName() | c->GetLevel() | c->GetPluginName();
        }
        os << table.Render();
    }
    return os;
}


