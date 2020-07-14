
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"

#include <JANA/Engine/JArrowTopology.h>
#include <JANA/Engine/JArrowProcessingController.h>
#include "PerformanceTests.h"
#include "TestTopology.h"

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

    TestTopology topology;

    auto q1 = new JMailbox<Event*>();
    auto q2 = new JMailbox<Event*>();
    auto q3 = new JMailbox<Event*>();

    topology.addArrow(new SourceArrow<Event*>("parse", parse, q1));
    topology.addArrow(new MapArrow<Event*,Event*>("disentangle", disentangle, q1, q2));
    topology.addArrow(new MapArrow<Event*,Event*>("track", track, q2, q3));
    topology.addArrow(new SinkArrow<Event*>("plot", plot, q3));

    JArrowTopology proctop;
    proctop.arrows = std::move(topology.arrows);
    proctop.sources.push_back(proctop.arrows[0]);
    proctop.sinks.push_back(proctop.arrows[3]);

    JArrowProcessingController controller(&proctop);
    controller.initialize();

    for (int nthreads=1; nthreads<6; nthreads++) {
        controller.run(nthreads);
        for (int secs=0; secs<10; secs++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            controller.print_report();
        }
        controller.wait_until_stopped();
        //auto result = controller.measure_internal_performance();
        //std::cout << nthreads << ": " << result->avg_throughput_hz << " Hz" << std::endl;

    }
}


