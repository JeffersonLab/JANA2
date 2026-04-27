
#include <catch.hpp>
#include <JANA/Topology/JArrow.h>

namespace jana::topology::jarrowtests {

struct TestData { int x; };

struct BasicParallelArrow : public JArrow {

    BasicParallelArrow() {
        AddPort("in", JEventLevel::PhysicsEvent);
        AddPort("out", JEventLevel::PhysicsEvent);
        SetIsParallel(true);
    }

    void Fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& process_status) override {

        input->Insert(new TestData {.x=22});

        outputs[0] = {input, 1};
        output_count = 1;
        process_status = JArrow::FireResult::KeepGoing;

    }
};


TEST_CASE("BasicParallelArrow_Fire") {

    JApplication app;
    app.Initialize();
    auto event = std::make_shared<JEvent>(&app);

    BasicParallelArrow sut;
    JArrow::OutputData outputs;
    size_t output_count;
    JArrow::FireResult status;

    sut.Fire(event.get(), outputs, output_count, status);

    REQUIRE(event->GetSingle<TestData>()->x == 22);
    REQUIRE(output_count == 1);
    REQUIRE(outputs[0].first == event.get());
    REQUIRE(outputs[0].second == 1);
    REQUIRE(status == JArrow::FireResult::KeepGoing);
}



TEST_CASE("BasicParallelArrow_ExecuteSucceeds") {

    JApplication app;
    app.Initialize();

    BasicParallelArrow sut;
    JEventPool input_pool(app.GetService<JComponentManager>(), 10, 1);
    JEventQueue output_queue(10, 1);

    sut.GetPort(0).Attach(&input_pool);
    sut.GetPort(1).Attach(&output_queue);

    auto result = sut.Execute( 0);

    REQUIRE(result == JArrow::FireResult::KeepGoing);
    REQUIRE(output_queue.GetSize(0) == 1);
    JEvent* event = output_queue.Pop(0);
    REQUIRE(event->GetSingle<TestData>()->x == 22);
}


TEST_CASE("BasicParallelArrow_ExecuteFails") {

    JApplication app;
    app.Initialize();

    BasicParallelArrow sut;
    JEventQueue input_queue(10, 1);
    JEventQueue output_queue(10, 1);

    sut.GetPort(0).Attach(&input_queue);
    sut.GetPort(1).Attach(&output_queue);

    auto result = sut.Execute(0);

    REQUIRE(result == JArrow::FireResult::NotRunYet);
    REQUIRE(output_queue.GetSize(0) == 0);
}



} // namespace jana::topology::jarrowtests
