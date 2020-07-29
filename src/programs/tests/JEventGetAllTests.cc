
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"
#include <JANA/JEvent.h>

struct Base {
    double base;
    Base(double base) : base(base) {};
};

struct Derived : public Base {
    double derived;
    Derived(double base, double derived) : Base(base), derived(derived) {};
};

struct Unrelated {
    double unrelated;
    Unrelated(double unrelated) : unrelated(unrelated) {};
};

struct Multiple : public Derived, Unrelated {
    double multiple;
    Multiple(double base, double derived, double unrelated, double multiple)
        : Derived(base, derived), Unrelated(unrelated), multiple(multiple) {};
};

class DerivedFactory : public JFactoryT<Derived> {
public:
    DerivedFactory() {
        EnableGetAs<Base>();
    }
};

class OtherDerivedFactory : public JFactoryT<Derived> {
public:
    OtherDerivedFactory() {
        EnableGetAs<Base>();
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {
        Insert(new Derived(19, 13));
        Insert(new Derived(29, 23));
    }
};

class UnrelatedFactory : public JFactoryT<Unrelated> {};

class MultipleFactory : public JFactoryT<Multiple> {
public:
    MultipleFactory() {
        EnableGetAs<Base>();
        EnableGetAs<Derived>();
        EnableGetAs<Unrelated>();
    }
};

class MultipleFactoryMissing : public JFactoryT<Multiple> {
public:
    MultipleFactoryMissing() {
        EnableGetAs<Derived>();
    }
};

TEST_CASE("JFactoryGetAs") {

    SECTION("Upcast Derived to Base (single item inserted)") {
        DerivedFactory f;
        f.Insert(new Derived (7, 22));
        auto bases = f.GetAs<Base>();
        REQUIRE(bases.size() == 1);
        REQUIRE(bases[0]->base == 7);
    }

    SECTION("Upcast Derived to Base (multiple items inserted)") {
        DerivedFactory f;
        f.Insert(new Derived (7, 22));
        f.Insert(new Derived (8, 23));
        f.Insert(new Derived (9, 24));
        auto bases = f.GetAs<Base>();

        REQUIRE(bases.size() == 3);
        REQUIRE(bases[0]->base == 7);
        REQUIRE(bases[1]->base == 8);
        REQUIRE(bases[2]->base == 9);
    }

    SECTION("Upcast Derived to Unrelated (single item inserted)") {
        DerivedFactory f;
        f.Insert(new Derived (7, 22));
        auto bases = f.GetAs<Unrelated>();
        REQUIRE(bases.size() == 0);
        REQUIRE(bases.size() == 0);
    }

    SECTION("Upcast Derived to Derived (single item inserted)") {
        DerivedFactory f;
        f.Insert(new Derived(22, 18));
        auto deriveds = f.GetAs<Derived>();
        REQUIRE(deriveds.size() == 1);
        REQUIRE(deriveds[0]->derived == 18);
    }

    SECTION("GetAs doesn't trigger Process()") {
        OtherDerivedFactory f;
        auto deriveds_before = f.GetAs<Derived>();
        REQUIRE(deriveds_before.size() == 0);
        f.Process(std::make_shared<JEvent>());
        auto deriveds_after = f.GetAs<Derived>();
        REQUIRE(deriveds_after.size() == 2);
    }

    SECTION("Upcast Multiple to each of its bases") {
        MultipleFactory f;
        f.Insert(new Multiple(22, 27, 42, 49));

        auto bases = f.GetAs<Base>();
        REQUIRE(bases.size() == 1);
        REQUIRE(bases[0]->base == 22);

        auto deriveds = f.GetAs<Derived>();
        REQUIRE(deriveds.size() == 1);
        REQUIRE(deriveds[0]->derived == 27);

        auto unrelateds = f.GetAs<Unrelated>();
        REQUIRE(unrelateds.size() == 1);
        REQUIRE(unrelateds[0]->unrelated == 42);

        auto multiples = f.GetAs<Multiple>();
        REQUIRE(multiples.size() == 1);
        REQUIRE(multiples[0]->multiple == 49);
    }

    SECTION("Upcast only succeeds if bases have been enabled") {
        MultipleFactoryMissing f;
        f.Insert(new Multiple(22, 27, 42, 49));

        auto bases = f.GetAs<Base>();
        REQUIRE(bases.size() == 0);

        auto deriveds = f.GetAs<Derived>();
        REQUIRE(deriveds.size() == 1);
        REQUIRE(deriveds[0]->derived == 27);

        auto unrelateds = f.GetAs<Unrelated>();
        REQUIRE(unrelateds.size() == 0);

        auto multiples = f.GetAs<Multiple>();
        REQUIRE(multiples.size() == 1);
        REQUIRE(multiples[0]->base == 22);
        REQUIRE(multiples[0]->derived == 27);
        REQUIRE(multiples[0]->unrelated == 42);
        REQUIRE(multiples[0]->multiple == 49);
    }
}

TEST_CASE("JEventGetAllChildren") {

    auto event = std::make_shared<JEvent>();
    event->SetFactorySet(new JFactorySet);

    SECTION("Single-item JEvent::Insert() can be retrieved via JEvent::GetAllChildren()") {
        auto b = new Base(22);
        auto d = new Derived(33,44);
        auto u = new Unrelated(19);
        auto m = new Multiple(1,2,3,4);

        event->Insert(b);
        event->Insert(u);
        event->Insert(d)->EnableGetAs<Base>();

        auto f = event->Insert(m);
        f->EnableGetAs<Base>();
        f->EnableGetAs<Derived>();
        f->EnableGetAs<Unrelated>();

        auto base_map = event->GetAllChildren<Base>();
        REQUIRE(base_map.size() == 3);
        REQUIRE(base_map[{"Base",""}].size() == 1);
        REQUIRE(base_map[{"Base",""}][0]->base == 22);
        REQUIRE(base_map[{"Derived",""}].size() == 1);
        REQUIRE(base_map[{"Derived",""}][0]->base == 33);
        REQUIRE(base_map[{"Multiple",""}].size() == 1);
        REQUIRE(base_map[{"Multiple",""}][0]->base == 1);
    }

}
