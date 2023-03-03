
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"
#include <JANA/JMultifactory.h>

namespace multifactory_tests {

struct A {
    float x, y;
};

struct B {
    int en, rn;  // Event number, run number
};

class MyMultifactory : public JMultifactory {

    bool m_set_wrong_output = false;

public:
    MyMultifactory(bool set_wrong_output) : m_set_wrong_output(set_wrong_output) {
        DeclareOutput<A>("first");
        DeclareOutput<B>("second");
    }

    void Process(const std::shared_ptr<const JEvent>&) {
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
    MyMultifactory sut(false);

    // Test calling the wrong SetData()
    // Test calling from within JEvent
    // Test adding to a JApplication via the usual JFactoryGeneratorT
    // Test that caching happens when it is supposed to
    // Test that janadot still works
    // Test that


}

} // namespace multifactory_tests
