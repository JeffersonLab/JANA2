
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

struct FacA: public JFactoryT<PodX> {
    void Process(const std::shared_ptr<const JEvent>& ) override {
        Insert(new PodX(22.2));
        Insert(new PodX(0.8));
    }
};
struct FacB: public JFactoryT<PodX> {
    void Process(const std::shared_ptr<const JEvent>& ) override {
        Insert(new PodX(33.3));
        Insert(new PodX(0.7));
    }
};
struct FacC: public JFactoryT<PodY> {
    std::string podx_tag = "DetA:PodY:FacC";

    void Process(const std::shared_ptr<const JEvent>& event) override {
        auto podas = event->Get<PodX>(podx_tag);   // Which PodA does it get?
        double E_tot = 0;
        for (auto* poda : podas) {
            E_tot += poda->E;
        }
        Insert(new PodY(E_tot));
    }
};
struct FacD: public JFactoryT<PodZ> {
    std::string pody_tag = "DetA:PodZ:FacD";

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
        return results;
    }

    std::string GetConcreteCalleeTag(std::string concrete_caller_tag, std::string abstract_callee_tag) {
        auto caller_node = m_concrete_tag_to_node[concrete_caller_tag];
        auto concrete_callee_tag = caller_node->GetDependency(abstract_callee_tag)->GetConcreteTag();
        return concrete_callee_tag;
    }
};

class WiredFactoryGenerator : public JFactoryGenerator {
    JWiring wiring;

public:
    WiredFactoryGenerator(JWiring wiring) : wiring(std::move(wiring)) {};

    void GenerateFactories(JFactorySet *factory_set) override {

        // Generate all necessary FacA's
        auto fac_a_tags = wiring.GetConcreteTagsForFactory("FacA");
        for (auto caller_tag: fac_a_tags) {
            auto fac = new FacA;
            fac->SetTag(caller_tag);
            factory_set->Add(fac);
        }

        // Generate all necessary FacB's
        auto fac_b_tags = wiring.GetConcreteTagsForFactory("FacB");
        for (auto caller_tag: fac_b_tags) {
            auto fac = new FacB;
            fac->SetTag(caller_tag);
            factory_set->Add(fac);
        }

        // Generate all necessary FacC's
        auto fac_c_tags = wiring.GetConcreteTagsForFactory("FacC");
        for (auto caller_tag: fac_c_tags) {
            auto fac = new FacC;
            fac->SetTag(caller_tag);
            fac->podx_tag = wiring.GetConcreteCalleeTag(caller_tag, "DetJPodX");
            factory_set->Add(fac);
        }

        // Generate all necessary FacD's
        auto fac_d_tags = wiring.GetConcreteTagsForFactory("FacD");
        for (auto caller_tag: fac_d_tags) {
            auto fac = new FacD;
            fac->SetTag(caller_tag);
            fac->pody_tag = wiring.GetConcreteCalleeTag(caller_tag, "DetJPodY");
            factory_set->Add(fac);
        }

    }
};

TEST_CASE("ExplicitWiringTestsWithoutDefaults") {
    JWiring wiring;
    auto facA = wiring.DeclareFactoryInstance("PodX","FacA",0);
    auto facB = wiring.DeclareFactoryInstance("PodX","FacB",0);
    auto facC = wiring.DeclareFactoryInstance("PodY","FacC",0)
            ->AddDependency("DetJPodX", facA);
    auto facD = wiring.DeclareFactoryInstance("PodZ","FacD",0)
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
