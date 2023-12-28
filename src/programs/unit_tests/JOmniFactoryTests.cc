
#include <catch.hpp>
#include <JANA/Experimental/JEventHierarchy.h>

namespace jana2 {
namespace omnifactorytests {

TEST_CASE("OmniFactoryTests_RetrieveInsert") {

    JBlock block {nullptr, 18};
    JPhysicsEvent event {&block, 22};

    auto* coll = new JBasicCollection<int>("ints");
    (*coll)().push_back(new int(33));

    event.Insert(coll);

    auto x = event.GetOrCreate<JBasicCollection<int>>("ints");
    REQUIRE((*x)(0) == 33);
}

struct TestFac : public JOmniFactory<JPhysicsEvent> {
    Input<JPhysicsEvent, JBasicCollection<int>> ints_in {this, "ints"};
    Output<JBasicCollection<double>> doubles_out {this, "doubles"};

    void Process(JPhysicsEvent& event) {
        for (const int* item : ints_in()) {
            doubles_out().push_back(new double(*item + 22.2));
        }
    }
};

TEST_CASE("OmniFactoryTests_FacRetrievesInsert") {

    JBlock block {nullptr, 18};
    JPhysicsEvent event {&block, 22};

    auto* coll = new JBasicCollection<int>("ints");
    (*coll)().push_back(new int(33));

    event.Insert(coll);
    event.AddFactory(std::make_unique<TestFac>());

    auto x = event.GetOrCreate<JBasicCollection<double>>("doubles");
    REQUIRE((*x)(0) == 55.2);
}

struct TestFac2 : public JOmniFactory<JPhysicsEvent> {
    Input<JBlock, JBasicCollection<int>> ints_in {this, "ints"};
    Output<JBasicCollection<double>> doubles_out {this, "doubles"};

    void Process(JPhysicsEvent& event) {
        for (const int* item : ints_in()) {
            doubles_out().push_back(new double(*item + 22.2));
        }
    }
};

struct PlusOneFac : public JOmniFactory<JBlock> {
    Input<JBlock, JBasicCollection<int>> ints_in {this, "ints_before"};
    Output<JBasicCollection<int>> ints_out {this, "ints"};

    void Process(JBlock& block) {
        for (const int* item : ints_in()) {
            ints_out().push_back(new int(*item + 1));
        }
    }
};

TEST_CASE("OmniFactoryTests_EvtFacRetrievesBlockFac") {

    JBlock block {nullptr, 18};
    JPhysicsEvent event {&block, 22};

    auto* coll = new JBasicCollection<int>("ints_before");
    (*coll)().push_back(new int(33));

    block.Insert(coll);
    block.AddFactory(std::make_unique<PlusOneFac>());
    event.AddFactory(std::make_unique<TestFac2>());

    auto x = event.GetOrCreate<JBasicCollection<double>>("doubles");
    REQUIRE((*x)(0) == 56.2);

    auto y = block.Get<JBasicCollection<int>>("ints_before");
    REQUIRE((*y)(0) == 33);

    auto z = block.Get<JBasicCollection<int>>("ints");
    REQUIRE((*z)(0) == 34);
}

// TEST_CASE("OmniFactoryTests_InsertOverridesFac") {
//
// }

} // namespace omnifactorytests
} // namespace jana2
