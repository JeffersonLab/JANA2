
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"
#include "JANA/JFactoryGenerator.h"
#include <JANA/JMultifactory.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/JEvent.h>

namespace multifactory_tests {

struct A {
    float x, y;
};

struct B {
    int en, rn;  // Event number, run number
};

struct MyMultifactory : public JMultifactory {

    bool m_set_wrong_output = false;
    int m_process_call_count = 0;
    int m_init_call_count = 0;

public:
    MyMultifactory(bool set_wrong_output=false) : m_set_wrong_output(set_wrong_output) {
        DeclareOutput<A>("first");
        DeclareOutput<B>("second");
    }

    void Init() override {
        auto app = GetApplication();
        REQUIRE(app != nullptr);
        m_init_call_count += 1;
    }

    void Process(const std::shared_ptr<const JEvent>&) override {
        m_process_call_count += 1;
        std::vector<A*> as;
        std::vector<B*> bs;
        as.push_back(new A {3.3, 4.4});
        as.push_back(new A {5.5, 6.6});
        bs.push_back(new B {1,1});
        SetData("first", as);
        SetData("second", bs);
        if (m_set_wrong_output) SetData("third", bs);
    }
};

TEST_CASE("MultiFactoryTests") {
    JApplication app;


    SECTION("Calling from JEvent") {
        auto sut = new MyMultifactory(false);
        auto event = std::make_shared<JEvent>(&app);
        auto fs = new JFactorySet;
        fs->Add(sut);
        event->SetFactorySet(fs);
        auto as = event->Get<A>("first");
        REQUIRE(as.size() == 2);
        REQUIRE(as[1]->x == 5.5);

        auto bs = event->Get<B>("second");
        REQUIRE(bs.size() == 1);
        REQUIRE(bs[0]->en == 1);
    }

    SECTION("Multifactory sets the wrong data") {
        auto sut = new MyMultifactory(true);
        auto event = std::make_shared<JEvent>(&app);
        auto fs = new JFactorySet;
        fs->Add(sut);
        event->SetFactorySet(fs);
        REQUIRE_THROWS(event->Get<A>("first"));
    }

    SECTION("Multifactories work with JFactoryGeneratorT") {
        app.Add(new JFactoryGeneratorT<MyMultifactory>());
        auto jcm = app.GetService<JComponentManager>();
        auto event = std::make_shared<JEvent>(&app);
        jcm->configure_event(*event);
        auto as = event->Get<A>("first");
        REQUIRE(as.size() == 2);
        REQUIRE(as[1]->x == 5.5);
    }

    SECTION("Test that multifactory Process() is only called once") {
        app.Add(new JFactoryGeneratorT<MyMultifactory>());
        auto jcm = app.GetService<JComponentManager>();
        auto event = std::make_shared<JEvent>(&app);
        jcm->configure_event(*event);

        auto helper_fac = dynamic_cast<JMultifactoryHelper<A>*>(event->GetFactory<A>("first"));
        REQUIRE(helper_fac != nullptr);
        auto sut = dynamic_cast<MyMultifactory*>(helper_fac->GetMultifactory());
        REQUIRE(sut != nullptr);

        REQUIRE(sut->m_process_call_count == 0);
        REQUIRE(sut->m_init_call_count == 0);

        auto as = event->Get<A>("first");
        REQUIRE(as.size() == 2);
        REQUIRE(as[1]->x == 5.5);
        REQUIRE(sut->m_init_call_count == 1);
        REQUIRE(sut->m_process_call_count == 1);

        auto bs = event->Get<B>("second");
        REQUIRE(bs.size() == 1);
        REQUIRE(bs[0]->en == 1);
        REQUIRE(sut->m_init_call_count == 1);
        REQUIRE(sut->m_process_call_count == 1);

        as = event->Get<A>("first");
        REQUIRE(as.size() == 2);
        REQUIRE(as[1]->x == 5.5);
        REQUIRE(sut->m_init_call_count == 1);
        REQUIRE(sut->m_process_call_count == 1);
    }

}

} // namespace multifactory_tests
