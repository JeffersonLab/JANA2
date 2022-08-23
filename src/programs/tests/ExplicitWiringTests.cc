
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <catch.hpp>
#include <JANA/JApplication.h>
#include <JANA/JFactoryT.h>
#include <JANA/JObject.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Services/JComponentManager.h>

namespace jana {
namespace explicit_wiring_tests {

class Autowirable {
    friend class JWiring;
    std::map<std::string, std::string*> m_autowires;

public:
    void Autowire(const std::string& abstract_tag, std::string& member) {
        m_autowires[abstract_tag] = &member;
    }
};


struct PodX: public JObject {
    JOBJECT_PUBLIC(PodX);
    PodX(double E): E(E) {};
    double E = 0.0;
};

struct PodY: public JObject {
    JOBJECT_PUBLIC(PodY);
    PodY(double E_tot): E_tot(E_tot) {};
    double E_tot = 0.0;
};

struct PodZ: public JObject {
    JOBJECT_PUBLIC(PodZ);
    PodZ(bool trigger): trigger(trigger) {};
    bool trigger = false;
};

struct FacA: public JFactoryT<PodX>, Autowirable {
    void Process(const std::shared_ptr<const JEvent>& ) override {
        Insert(new PodX(22.2));
        Insert(new PodX(0.8));
    }
};
struct FacB: public JFactoryT<PodX>, Autowirable {
    void Process(const std::shared_ptr<const JEvent>& ) override {
        Insert(new PodX(33.3));
        Insert(new PodX(0.7));
    }
};
struct FacC: public JFactoryT<PodY>, Autowirable {
    std::string podx_tag = "DetA:PodY:FacC";

    FacC() {
        Autowire("DetJPodX", podx_tag);
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {
        auto podas = event->Get<PodX>(podx_tag);   // Which PodA does it get?
        double E_tot = 0;
        for (auto* poda : podas) {
            E_tot += poda->E;
        }
        Insert(new PodY(E_tot));
    }
};
struct FacD: public JFactoryT<PodZ>, Autowirable {
    std::string pody_tag = "DetA:PodZ:FacD";

    FacD() {
        Autowire("DetJPodY", pody_tag);
    }
    void Process(const std::shared_ptr<const JEvent>& event) override {
        auto podb = event->GetSingle<PodY>(pody_tag);
        if (podb->E_tot > 25) {
            Insert(new PodZ(true));
        }
        else {
            Insert(new PodZ(false));
        }
    }
};


TEST_CASE("ExplicitWiringTests") {

    JApplication app;
    app.Add(new JFactoryGeneratorT<FacA>("DetA:PodX:FacA"));
    app.Add(new JFactoryGeneratorT<FacB>("DetA:PodX:FacB"));
    app.Add(new JFactoryGeneratorT<FacC>());

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);
    event->SetJApplication(&app);

    auto item = event->Get<PodY>();
    REQUIRE(item[0]->E_tot == 23.0);
}

class JWiring {
public:
    class Node {
        JWiring &m_wiring;
        std::string m_obj_typename;
        std::string m_fac_typename;
        int m_fac_instance;

        // Concrete collection names are of the format fac_typename:fac_instance
        std::map<std::string, Node *> m_dependencies;  // Maps abstract collection names to concrete collection names
    public:
        Node(JWiring &wiring, std::string obj_typename, std::string fac_typename, int fac_instance=0)
        : m_wiring(wiring)
        , m_obj_typename(std::move(obj_typename))
        , m_fac_typename(std::move(fac_typename))
        , m_fac_instance(fac_instance)
        {
        }

        Node* AddDependency(std::string abstract_tag, Node* node) {
            m_dependencies[std::move(abstract_tag)] = node;
            return this;
        }
        JWiring& Finish() {
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

class WiredFactoryGenerator : public JFactoryGenerator {
    JWiring wiring;

public:
    explicit WiredFactoryGenerator(JWiring wiring) : wiring(std::move(wiring)) {};

    void GenerateFactories(JFactorySet *factory_set) override {
        wiring.GenerateAndWire<FacA>(factory_set);
        wiring.GenerateAndWire<FacB>(factory_set);
        wiring.GenerateAndWire<FacC>(factory_set);
        wiring.GenerateAndWire<FacD>(factory_set);
    }
};

TEST_CASE("ExplicitWiringTestsWithoutDefaults") {
    JWiring wiring;
    auto facA = wiring.DeclareFactoryInstance("PodX", JTypeInfo::demangle<FacA>(),0);
    auto facB = wiring.DeclareFactoryInstance("PodX", JTypeInfo::demangle<FacB>(),0);
    auto facC = wiring.DeclareFactoryInstance("PodY", JTypeInfo::demangle<FacC>(),0)
            ->AddDependency("DetJPodX", facA);
    auto facD = wiring.DeclareFactoryInstance("PodZ", JTypeInfo::demangle<FacD>(),0)
            ->AddDependency("DetJPodY", facC);

    JApplication app;
    app.Add(new WiredFactoryGenerator(wiring));

    auto event = std::make_shared<JEvent>();
    app.GetService<JComponentManager>()->configure_event(*event);
    event->SetJApplication(&app);

    auto pody_tag = wiring.GetConcreteCalleeTag(facD->GetConcreteTag(), "DetJPodY");
    auto item = event->Get<PodY>(pody_tag);
    REQUIRE(item[0]->E_tot == 23.0);
}


} // namespace explicit_wiring_tests
} // namespace jana
