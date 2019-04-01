
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
    PerfTestTopology topology;
    topology.logger = Logger(JLogLevel::TRACE, &std::cout);
    topology.activate("parse");
    topology.log_status();

    topology.run(4);

//    while (topology.is_active()) {
    for (int i=0; i<100; ++i) {  // This topology will run indefinitely
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "\033[2J";
        topology.log_status();
    }
}



