
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEXPLICITWIRINGSERVICE_H
#define JANA2_JEXPLICITWIRINGSERVICE_H

#include <JANA/Services/JServiceLocator.h>
#include <JANA/JFactorySet.h>

#include <vector>

class Autowirable {
    friend class JExplicitWiringService;
    std::map<std::string, std::string*> m_autowires;

public:
    void Autowire(const std::string& abstract_tag, std::string& member) {
        m_autowires[abstract_tag] = &member;
    }
};

class JExplicitWiringService : public JService {
public:
    class Node {
        JExplicitWiringService &m_wiring;
        std::string m_obj_typename;
        std::string m_fac_typename;
        int m_fac_instance;

        // Concrete collection names are of the format fac_typename:fac_instance
        std::map<std::string, Node *> m_dependencies;  // Maps abstract collection names to concrete collection names
    public:
        Node(JExplicitWiringService &wiring, std::string obj_typename, std::string fac_typename, int fac_instance=0)
                : m_wiring(wiring)
                , m_obj_typename(std::move(obj_typename))
                , m_fac_typename(std::move(fac_typename))
                , m_fac_instance(fac_instance)
        {
        }

        Node* WireDependency(std::string abstract_tag, Node* node) {
            m_dependencies[std::move(abstract_tag)] = node;
            return this;
        }
        JExplicitWiringService& Finish() {
            return m_wiring;
        }

        std::string GetConcreteTag() {
            std::ostringstream oss;
            oss << m_fac_typename << ":" << m_fac_instance;
            return oss.str();
        }

        Node* GetDependency(std::string abstract_tag) {
            return m_dependencies[abstract_tag];
        }

        std::string& GetFacTypename() {
            return m_fac_typename;
        }
    };

private:
    std::vector<Node*> m_nodes;
    std::multimap<std::string, std::string> m_fac_typename_to_concrete_tags;
    std::map<std::string, Node*> m_concrete_tag_to_node;

public:
    Node* DeclareFactoryInstance(std::string obj_typename, std::string fac_typename, int instance=0) {
        auto node = new Node(*this, obj_typename, fac_typename, instance);
        m_nodes.push_back(node);
        std::string concrete_tag = node->GetConcreteTag();
        if (m_concrete_tag_to_node.find(concrete_tag) != m_concrete_tag_to_node.end()) {
            throw JException("Duplicate concrete tag '%s'!", concrete_tag.c_str());
        }
        m_concrete_tag_to_node[concrete_tag] = node;
        m_fac_typename_to_concrete_tags.insert(std::make_pair(fac_typename, concrete_tag));
        return node;
    }


    std::vector<std::string> GetConcreteTagsForFactory(std::string fac_typename) {
        std::vector<std::string> results;
        auto tag_range = m_fac_typename_to_concrete_tags.equal_range(fac_typename);
        for (auto& tag=tag_range.first; tag != tag_range.second; ++tag) {
            results.push_back(tag->second);
        }
        if (results.empty()) {
            throw JException("JWiring does not know about JFactory with typename '%s'", fac_typename.c_str());
        }
        return results;
    }

    std::string GetConcreteCalleeTag(std::string concrete_caller_tag, std::string abstract_callee_tag) {
        auto caller_node = m_concrete_tag_to_node[concrete_caller_tag];
        auto concrete_callee_tag = caller_node->GetDependency(abstract_callee_tag)->GetConcreteTag();
        return concrete_callee_tag;
    }

    // T needs to be both a JFactory and an Autowirable
    template <typename T, typename... Args>
    void GenerateAndWire(JFactorySet* factorySet, Args... args) {
        for (auto caller_tag: GetConcreteTagsForFactory(JTypeInfo::demangle<T>())) {
            auto fac = new T(std::forward<Args>(args)...);
            fac->SetTag(caller_tag);
            for (auto pair : fac->m_autowires) {
                std::string* member = pair.second;
                *member = GetConcreteCalleeTag(caller_tag, pair.first);
            }
            factorySet->Add(fac);
        }
    }

};


#endif //JANA2_JEXPLICITWIRINGSERVICE_H
