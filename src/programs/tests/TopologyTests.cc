
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <TestTopologyComponents.h>
#include <JANA/Utils/JPerfUtils.h>
#include <JANA/Engine/JArrowTopology.h>


JArrowMetrics::Status step(JArrow* arrow) {
	JArrowMetrics metrics;
	arrow->execute(metrics, 0);
	auto status = metrics.get_last_status();
	if (status == JArrowMetrics::Status::Finished) {
            arrow->finish();
	}
	return status;
}

void log_status(JArrowTopology& topology) {

}



TEST_CASE("JTopology: Basic functionality") {

    RandIntSource source;
    MultByTwoProcessor p1;
    SubOneProcessor p2;
    SumSink<double> sink;

    JArrowTopology topology;

    auto q1 = new JMailbox<int>();
    auto q2 = new JMailbox<double>();
    auto q3 = new JMailbox<double>();

    auto emit_rand_ints = new SourceArrow<int>("emit_rand_ints", source, q1);
    auto multiply_by_two = new MapArrow<int,double>("multiply_by_two", p1, q1, q2);
    auto subtract_one = new MapArrow<double,double>("subtract_one", p2, q2, q3);
    auto sum_everything = new SinkArrow<double>("sum_everything", sink, q3);

	topology.sources.push_back(emit_rand_ints);

	topology.arrows.push_back(emit_rand_ints);
	topology.arrows.push_back(multiply_by_two);
	topology.arrows.push_back(subtract_one);
	topology.arrows.push_back(sum_everything);

	emit_rand_ints->set_chunksize(1);

    auto logger = JLogger(JLogger::Level::OFF);
    //topology.logger = Logger::everything();
    //source.logger = Logger::everything();


    SECTION("Before anything runs...") {

        // All queues are empty, none are finished
        REQUIRE(multiply_by_two->get_pending() == 0);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);
    }

    SECTION("When nothing is in the input queue...") {

        //LOG_INFO(logger) << "Nothing has run yet; should be empty" << LOG_END;

        //topology.log_queue_status();
        step(multiply_by_two);
        step(subtract_one);
        step(sum_everything);
        step(multiply_by_two);
        step(subtract_one);
        step(sum_everything);
        //topology.log_queue_status();

        // All `execute` operations are no-ops
        REQUIRE(multiply_by_two->get_pending() == 0);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);
    }

    SECTION("After emitting") {
        auto logger = JLogger(JLogger::Level::OFF);
        topology.m_logger = logger;
        source.logger = logger;

        //LOG_INFO(logger) << "After emitting; should be something in q0" << LOG_END;

        log_status(topology);
        topology.run();
        step(emit_rand_ints);
		log_status(topology);

        REQUIRE(multiply_by_two->get_pending() == 1);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);

        step(emit_rand_ints);
        step(emit_rand_ints);
        step(emit_rand_ints);
        step(emit_rand_ints);

        REQUIRE(multiply_by_two->get_pending() == 5);
        REQUIRE(subtract_one->get_pending() == 0);
        REQUIRE(sum_everything->get_pending() == 0);

    }

    SECTION("Running each stage sequentially yields the correct results") {

        //LOG_INFO(logger) << "Running each stage sequentially yields the correct results" << LOG_END;
        topology.run();

        for (int i = 0; i < 20; ++i) {
            step(emit_rand_ints);
            REQUIRE(multiply_by_two->get_pending() == 1);
            REQUIRE(subtract_one->get_pending() == 0);
            REQUIRE(sum_everything->get_pending() == 0);

            step(multiply_by_two);
            REQUIRE(multiply_by_two->get_pending() == 0);
            REQUIRE(subtract_one->get_pending() == 1);
            REQUIRE(sum_everything->get_pending() == 0);

            step(subtract_one);
            REQUIRE(multiply_by_two->get_pending() == 0);
            REQUIRE(subtract_one->get_pending() == 0);
            REQUIRE(sum_everything->get_pending() == 1);

            step(sum_everything);
            REQUIRE(multiply_by_two->get_pending() == 0);
            REQUIRE(subtract_one->get_pending() == 0);
            REQUIRE(sum_everything->get_pending() == 0);
        }

        REQUIRE(sink.sum == (7 * 2.0 - 1) * 20);
    }

    SECTION("Running each stage in random order (sequentially) yields the correct results") {
        //LOG_INFO(logger) << "Running each stage in arbitrary order yields the correct results" << LOG_END;

        JArrow* arrows[] = {emit_rand_ints,
                            multiply_by_two,
                            subtract_one,
                            sum_everything};

        std::map<std::string, JArrowMetrics::Status> results;
        results["emit_rand_ints"] = JArrowMetrics::Status::KeepGoing;
        results["multiply_by_two"] = JArrowMetrics::Status::KeepGoing;
        results["subtract_one"] = JArrowMetrics::Status::KeepGoing;
        results["sum_everything"] = JArrowMetrics::Status::KeepGoing;

        // Put something in the queue to get started
        topology.run();
        step(emit_rand_ints);

        bool work_left = true;
        while (work_left) {
            // Pick a random arrow
            JArrow* arrow = arrows[randint(0, 3)];

            auto name = arrow->get_name();
            JArrowMetrics metrics;
            arrow->execute(metrics, 0);
            auto res = metrics.get_last_status();
            results[name] = res;
            //LOG_TRACE(logger) << name << " => "
            //                  << to_string(res) << LOG_END;

            size_t pending = multiply_by_two->get_pending() +
                             subtract_one->get_pending() +
                             sum_everything->get_pending();

            work_left = (pending > 0);

            for (auto pair : results) {
                if (pair.second == JArrowMetrics::Status::KeepGoing) { work_left = true; }
            }
        }

        //topology.log_queue_status();
        REQUIRE(sink.sum == (7 * 2.0 - 1) * 20);
    }
    SECTION("Finished flag propagates") {

        logger = JLogger(JLogger::Level::OFF);
        topology.m_logger = logger;
        source.logger = logger;

        topology.run();

        REQUIRE(emit_rand_ints->get_status() == JActivable::Status::Running);
        REQUIRE(multiply_by_two->get_status() == JActivable::Status::Running);
        REQUIRE(subtract_one->get_status() == JActivable::Status::Running);
        REQUIRE(sum_everything->get_status() == JActivable::Status::Running);

        for (int i = 0; i < 20; ++i) {
            step(emit_rand_ints);
        }

        REQUIRE(emit_rand_ints->get_status() == JActivable::Status::Finished);
        REQUIRE(multiply_by_two->get_status() == JActivable::Status::Running);
        REQUIRE(subtract_one->get_status() == JActivable::Status::Running);
        REQUIRE(sum_everything->get_status() == JActivable::Status::Running);

        for (int i = 0; i < 20; ++i) {
            step(multiply_by_two);
        }

        REQUIRE(emit_rand_ints->get_status() == JActivable::Status::Finished);
        REQUIRE(multiply_by_two->get_status() == JActivable::Status::Stopped);
        REQUIRE(subtract_one->get_status() == JActivable::Status::Running);
        REQUIRE(sum_everything->get_status() == JActivable::Status::Running);

        for (int i = 0; i < 20; ++i) {
            step(subtract_one);
        }

        REQUIRE(emit_rand_ints->get_status() == JActivable::Status::Finished);
        REQUIRE(multiply_by_two->get_status() == JActivable::Status::Stopped);
        REQUIRE(subtract_one->get_status() == JActivable::Status::Stopped);
        REQUIRE(sum_everything->get_status() == JActivable::Status::Running);

        for (int i = 0; i < 20; ++i) {
            step(sum_everything);
        }

        REQUIRE(emit_rand_ints->get_status() == JActivable::Status::Finished);
        REQUIRE(multiply_by_two->get_status() == JActivable::Status::Stopped);
        REQUIRE(subtract_one->get_status() == JActivable::Status::Stopped);
        REQUIRE(sum_everything->get_status() == JActivable::Status::Stopped);

        log_status(topology);

    }
}
