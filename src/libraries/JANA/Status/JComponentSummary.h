
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Utils/JEventLevel.h>

#include <string>
#include <vector>
#include <map>
#include <memory>


// Keeping this around for backwards compatibility only
struct JFactorySummary {
    JEventLevel level;
    std::string plugin_name;
    std::string factory_name;
    std::string factory_tag;
    std::string object_name;
};


class JComponentSummary {
public:
    enum class ComponentType { Source, Processor, Factory, Unfolder, Folder };

    class Collection;

    class Component {
        friend class JComponentSummary;

        ComponentType m_component_type;
        std::string m_prefix;
        std::string m_type_name;
        JEventLevel m_level;
        std::string m_plugin_name;
        std::vector<Collection*> m_inputs;
        std::vector<Collection*> m_outputs;

    public:
        Component(ComponentType component_type, std::string prefix, std::string type_name, JEventLevel level, std::string plugin_name)
            : m_component_type(component_type), m_prefix(prefix), m_type_name(type_name), m_level(level), m_plugin_name(plugin_name) {}
        void AddInput(Collection* input) { m_inputs.push_back(input); }
        void AddOutput(Collection* output) { m_outputs.push_back(output); }
        ComponentType GetComponentType() const { return m_component_type; }
        std::string GetPrefix() const { return m_prefix; }
        std::string GetTypeName() const { return m_type_name; }
        JEventLevel GetLevel() const { return m_level; }
        std::string GetPluginName() const { return m_plugin_name; }
        std::vector<const Collection*> GetInputs() const { 
            std::vector<const Collection*> results;
            for (Collection* c : m_inputs) {
                results.push_back(c);
            }
            return results;
        }
        std::vector<const Collection*> GetOutputs() const { 
            std::vector<const Collection*> results;
            for (Collection* c : m_outputs) {
                results.push_back(c);
            }
            return results;
        }
    };

    class Collection {
        friend class JComponentSummary;

        std::string m_tag;
        std::string m_name;
        std::string m_type_name;
        JEventLevel m_level;
        std::vector<const Component*> m_producers;
        std::vector<const Component*> m_consumers;
    
    public:
        Collection(std::string tag, std::string name, std::string type_name, JEventLevel level)
            : m_tag(tag), m_name(name), m_type_name(type_name), m_level(level) {}
        void AddProducer(const Component* component) { m_producers.push_back(component); }
        void AddConsumer(const Component* component) { m_consumers.push_back(component); }
        std::string GetTag() const { return m_tag; }
        std::string GetName() const { return m_name; }
        std::string GetTypeName() const { return m_type_name; }
        JEventLevel GetLevel() const { return m_level; }
        std::vector<const Component*> GetProducers() const {
            return m_producers;
        }
        std::vector<const Component*> GetConsumers() const {
            return m_consumers;
        }
    };

private:
    std::vector<Component*> m_components;
    std::vector<Collection*> m_collections;
    std::map<std::string, std::vector<Component*>> m_component_lookups;
    std::map<std::string, std::vector<Collection*>> m_collection_lookups;

public:
    JComponentSummary() = default;

    JComponentSummary(const JComponentSummary&) = delete; // Otherwise we have a lot of internal pointers that have to be deep-copied

    ~JComponentSummary() {
        for (Component* c : m_components) {
            delete c;
        }
        for (Collection* c : m_collections) {
            delete c;
        }
    }



    void Add(Component* component);

    std::vector<const Collection*> GetAllCollections() const { 
        std::vector<const Collection*> results;
        for (auto* col: m_collections) {
            results.push_back(col);
        }
        return results; 
    }
    std::vector<const Component*> GetAllComponents() const { 
        std::vector<const Component*> results;
        for (auto* comp: m_components) {
            results.push_back(comp);
        }
        return results; 
    }

    std::vector<const Collection*> FindCollections(std::string collection_name="") const { 
        std::vector<const Collection*> results;
        auto it = m_collection_lookups.find(collection_name);
        if (it != m_collection_lookups.end()) {
            for (auto* col: m_collection_lookups.at(collection_name)) {
                results.push_back(col);
            }
        }
        return results;
    }

    std::vector<const Component*> FindComponents(std::string component_name="") const { 
        std::vector<const Component*> results;
        auto it = m_component_lookups.find(component_name);
        if (it != m_component_lookups.end()) {
            for (auto* comp: m_component_lookups.at(component_name)) {
                results.push_back(comp);
            }
        }
        return results; 
    }

public:
    // Kept for backwards compatibility
    std::vector<JFactorySummary> factories;
};

std::ostream& operator<<(std::ostream& os, const JComponentSummary& cs);
std::ostream& operator<<(std::ostream& os, JComponentSummary::Collection const&);
std::ostream& operator<<(std::ostream& os, JComponentSummary::Component const&);
void PrintComponentTable(std::ostream& os, const JComponentSummary&);
void PrintCollectionTable(std::ostream& os, const JComponentSummary&);

inline bool operator==(const JComponentSummary::Collection& lhs, const JComponentSummary::Collection& rhs) {
    return (lhs.GetLevel() == rhs.GetLevel()) && 
           (lhs.GetName() == rhs.GetName()) && 
           (lhs.GetTag() == rhs.GetTag()) && 
           (lhs.GetTypeName() == rhs.GetTypeName());
}

inline bool operator!=(const JComponentSummary::Collection& lhs, const JComponentSummary::Collection& rhs) { return !(lhs == rhs); }

