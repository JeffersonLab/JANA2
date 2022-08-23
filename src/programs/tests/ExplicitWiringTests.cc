
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <catch.hpp>

#include <JANA/JApplication.h>
#include <JANA/JFactoryT.h>
#include <JANA/JObject.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Services/JExplicitWiringService.h>

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


class WiredFactoryGenerator : public JFactoryGenerator {
    JExplicitWiringService& wiring;

public:
    explicit WiredFactoryGenerator(JExplicitWiringService& wiring) : wiring(wiring) {};

    void GenerateFactories(JFactorySet *factory_set) override {
        wiring.GenerateAndWire<FacA>(factory_set);
        wiring.GenerateAndWire<FacB>(factory_set);
        wiring.GenerateAndWire<FacC>(factory_set);
        wiring.GenerateAndWire<FacD>(factory_set);
    }
};

TEST_CASE("ExplicitWiringTestsWithoutDefaults") {
    JExplicitWiringService wiring;
    auto facA = wiring.DeclareFactoryInstance("PodX", JTypeInfo::demangle<FacA>(), 0);
    auto facB = wiring.DeclareFactoryInstance("PodX", JTypeInfo::demangle<FacB>(), 0);
    auto facC = wiring.DeclareFactoryInstance("PodY", JTypeInfo::demangle<FacC>(), 0)
            ->WireDependency("DetJPodX", facA);
    auto facD = wiring.DeclareFactoryInstance("PodZ", JTypeInfo::demangle<FacD>(), 0)
            ->WireDependency("DetJPodY", facC);
    // auto facC2 = wiring.DeclareFactoryInstance("PodY", JTypeInfo::demangle<FacC>(), 1)
    //         ->WireDependency("DetJPodX", facB);
    // auto facD2 = wiring.DeclareFactoryInstance("PodZ", JTypeInfo::demangle<FacD>(), 1)
    //         ->WireDependency("DetJPodY", facC2);

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
