
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <JANA/Engine/JArrowTopology.h>
#include <JANA/Engine/JArrowProcessingController.h>
#include "PerformanceTests.h"

TEST_CASE("MemoryBottleneckTest", "[.][performance]") {

    std::cout << "Running performance test" << std::endl;
    auto serviceLocator = new JServiceLocator();

    auto loggingService = std::make_shared<JLoggingService>();
    serviceLocator->provide(loggingService);

    loggingService->set_level("JThreadTeam", JLogger::Level::OFF);
    loggingService->set_level("JScheduler", JLogger::Level::OFF);
    loggingService->set_level("JTopology", JLogger::Level::INFO);

    auto params = std::make_shared<FakeParameterManager>();
    serviceLocator->provide(params);

    params->chunksize = 10;
    params->backoff_tries = 4;
    params->backoff_time = std::chrono::microseconds(50);
    params->checkin_time = std::chrono::milliseconds(400);

    PerfTestSource parse;
    PerfTestMapper disentangle;
    PerfTestMapper track;
    PerfTestReducer plot;

    parse.message_count_limit = 5000;
    parse.latency_ms = 10;
    disentangle.latency_ms = 10;
    track.latency_ms = 10;
    plot.latency_ms = 10;

//    5Hz for full reconstruction
//    20kHz for stripped-down reconstruction (not per-core)
//    Base memory allcation: 100 MB/core + 600MB
//    1 thread/event, disentangle 1 event, turn into 40.
//    disentangled (single event size) : 12.5 kB / event (before blown up)
//    entangled "block of 40": dis * 40
//

    auto topology = std::make_shared<JArrowTopology>();

    auto q1 = new JMailbox<Event*>();
    auto q2 = new JMailbox<Event*>();
    auto q3 = new JMailbox<Event*>();

    auto parse_arrow = new SourceArrow<Event*>("parse", parse, q1);
    auto disentangle_arrow = new MapArrow<Event*,Event*>("disentangle", disentangle, q1, q2);
    auto track_arrow = new MapArrow<Event*,Event*>("track", track, q2, q3);
    auto plot_arrow = new SinkArrow<Event*>("plot", plot, q3);

    parse_arrow->attach(disentangle_arrow);
    disentangle_arrow->attach(track_arrow);
    track_arrow->attach(plot_arrow);

    parse_arrow->set_chunksize(1);

    topology->sources.push_back(parse_arrow);
    topology->sinks.push_back(plot_arrow);
    topology->arrows.push_back(parse_arrow);
    topology->arrows.push_back(disentangle_arrow);
    topology->arrows.push_back(track_arrow);
    topology->arrows.push_back(plot_arrow);

    JArrowProcessingController controller(topology);
    controller.initialize();

    controller.run(6); // for whatever mysterious reason we need to pre-warm our thread team
    for (int nthreads=1; nthreads<6; nthreads++) {
        controller.scale(nthreads);
        for (int secs=0; secs<10; secs++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        controller.request_pause();
        controller.wait_until_paused();
        auto result = controller.measure_internal_performance();
        std::cout << nthreads << ": " << result->avg_throughput_hz << " Hz" << std::endl;

    }
    controller.scale(1);
    controller.request_stop();
    controller.wait_until_stopped();
    controller.print_final_report();
    auto perf = controller.measure_internal_performance();
    // REQUIRE(perf->total_events_completed == perf->arrows[0].total_messages_completed);
}


