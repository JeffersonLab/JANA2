//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//
#include "catch.hpp"

#include "PerformanceTests.h"
#include "TestTopologyBuilder.h"

TEST_CASE("MemoryBottleneckTest", "[.][performance]") {

    std::cout << "Running performance test" << std::endl;
    serviceLocator = new JServiceLocator();

    auto loggingService = new JLoggingService;
    serviceLocator->provide(loggingService);

    loggingService->set_level("JThreadTeam", JLogLevel::OFF);
    loggingService->set_level("JScheduler", JLogLevel::OFF);
    loggingService->set_level("JTopology", JLogLevel::INFO);

    auto params = new ParameterManager;
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
    TestTopologyBuilder builder(topology);

    builder.addSource("parse", parse);
    builder.addProcessor("disentangle", disentangle);
    builder.addProcessor("track", track);
    builder.addSink("plot", plot);

    topology.activate("parse");
    //topology.log_status();
    topology.run(4);

    while (topology.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        std::cout << "\033[2J";
        topology.log_status();
    }

    topology.wait_until_finished();
    topology.log_status();
}


