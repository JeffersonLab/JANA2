
#include <catch.hpp>
#include <JANA/Topology/JTriggeredArrow.h>

namespace jana::topology::jarrowtests {

struct TestData { int x; };

struct BasicParallelArrow : public JTriggeredArrow<BasicParallelArrow> {

    Place m_input {this, true };
    Place m_output {this, false };

    BasicParallelArrow() {
        set_is_parallel(true);
    }

    void fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrowMetrics::Status& process_status) {

        input->Insert(new TestData {.x=22});

        outputs[0] = {input, 1};
        output_count = 1;
        process_status = JArrowMetrics::Status::KeepGoing;

    }
};


TEST_CASE("BasicParallelArrow_Fire") {

    JApplication app;
    app.Initialize();
    auto event = std::make_shared<JEvent>(&app);

    BasicParallelArrow sut;
    JArrow::OutputData outputs;
    size_t output_count;
    JArrowMetrics::Status status;

    sut.fire(event.get(), outputs, output_count, status);

    REQUIRE(event->GetSingle<TestData>()->x == 22);
    REQUIRE(output_count == 1);
    REQUIRE(outputs[0].first == event.get());
    REQUIRE(outputs[0].second == 1);
    REQUIRE(status == JArrowMetrics::Status::KeepGoing);
}



TEST_CASE("BasicParallelArrow_ExecuteSucceeds") {

    JApplication app;
    app.Initialize();

    BasicParallelArrow sut;
    JEventPool input_pool(app.GetService<JComponentManager>(), 10, 1);
    JMailbox<JEvent*> output_queue(10);

    sut.attach(&input_pool, 0);
    sut.attach(&output_queue, 1);

    JArrowMetrics metrics;
    sut.execute(metrics, 0);

    REQUIRE(metrics.get_last_status() == JArrowMetrics::Status::KeepGoing);
    REQUIRE(metrics.get_total_message_count() == 1);
    REQUIRE(output_queue.size() == 1);
    JEvent* event;
    output_queue.pop(&event, 1, 1, 0);
    REQUIRE(event->GetSingle<TestData>()->x == 22);
}


TEST_CASE("BasicParallelArrow_ExecuteFails") {

    JApplication app;
    app.Initialize();

    BasicParallelArrow sut;
    JMailbox<JEvent*> input_queue(10);
    JMailbox<JEvent*> output_queue(10);

    sut.attach(&input_queue, 0);
    sut.attach(&output_queue, 1);

    JArrowMetrics metrics;
    sut.execute(metrics, 0);

    REQUIRE(metrics.get_last_status() == JArrowMetrics::Status::ComeBackLater);
    REQUIRE(metrics.get_total_message_count() == 0);
    REQUIRE(output_queue.size() == 0);
}



} // namespace jana::topology::jarrowtests
