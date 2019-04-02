
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

    parse.latency_ms = 10;
    disentangle.latency_ms = 10;
    track.latency_ms = 10;
    plot.latency_ms = 10;

    LinearTopologyBuilder builder;
    builder.addSource("parse", parse);
    builder.addProcessor("disentangle", disentangle);
    builder.addProcessor("track", track);
    builder.addSink("plot", plot);

    Topology topology = builder.get();
    topology.logger = Logger(JLogLevel::TRACE, &std::cout);
    topology.activate("parse");
    topology.log_status();

    topology.run(4);

//    while (topology.is_active()) {
    for (int i=0; i<50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "\033[2J";
        topology.log_status();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    topology.log_status();
}



