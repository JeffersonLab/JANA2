
#include <greenfield/Logger.h>
#include <greenfield/Topology.h>
#include <greenfield/ThreadManager.h>
#include <greenfield/Worker.h>
#include <greenfield/ExampleComponents.h>
#include <greenfield/LinearTopologyBuilder.h>
#include <greenfield/PerfTestTopology.h>

using namespace std;
using namespace greenfield;


int main() {

    PerfTestSource parse;
    PerfTestMapper disentangle;
    PerfTestMapper track;
    PerfTestReducer plot;

    parse.message_count_limit = 500;
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

    Topology topology;

    LinearTopologyBuilder builder(topology);

    builder.addSource("parse", parse);
    builder.addProcessor("disentangle", disentangle);
    builder.addProcessor("track", track);
    builder.addSink("plot", plot);

    topology.logger = Logger(JLogLevel::TRACE, &std::cout);
    topology.activate("parse");

    topology.log_status();
    topology.run(4);

    //while (topology.is_active()) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        //std::cout << "\033[2J";
        //topology.log_status();
    //}

    topology.wait_until_finished();
    topology.log_status();
}



