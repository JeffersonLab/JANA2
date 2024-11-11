
#include <catch.hpp>
#include <JANA/Topology/JArrow.h>

namespace jana::topology::jarrowtests {

struct TestData { int x; };

struct BasicParallelArrow : public JArrow {

    BasicParallelArrow() {
        create_ports(1, 1);
        set_is_parallel(true);
    }

    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& process_status) {

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

    sut.fire(event.get(), outputs, output_count, status);

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

    sut.attach(&input_pool, 0);
    sut.attach(&output_queue, 1);

    JArrowMetrics metrics;
    sut.execute(metrics, 0);

    REQUIRE(metrics.get_last_status() == JArrow::FireResult::KeepGoing);
    REQUIRE(metrics.get_total_message_count() == 1);
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

    sut.attach(&input_queue, 0);
    sut.attach(&output_queue, 1);

    JArrowMetrics metrics;
    sut.execute(metrics, 0);

    REQUIRE(metrics.get_last_status() == JArrow::FireResult::ComeBackLater);
    REQUIRE(metrics.get_total_message_count() == 0);
    REQUIRE(output_queue.GetSize(0) == 0);
}



} // namespace jana::topology::jarrowtests
