
#include <greenfield/Logger.h>
#include <greenfield/Topology.h>
#include <greenfield/ThreadManager.h>
#include <greenfield/Worker.h>
#include <greenfield/ExampleComponents.h>
#include <greenfield/LinearTopologyBuilder.h>
#include <greenfield/PerfTestTopology.h>

using namespace std;
using namespace greenfield;


std::string stringify(ThreadManager::Response response) {
    switch (response) {
        case ThreadManager::Response::Success : return "Success";
        case ThreadManager::Response::AlreadyRunning: return "AlreadyRunning";
        default: return "Other";
    }

}

int main() {
    LoggingService loggingService;
    loggingService.set_level(JLogLevel::INFO);
    loggingService.set_level("RoundRobinScheduler", JLogLevel::INFO);
    loggingService.set_level("ThreadManager", JLogLevel::INFO);
    auto logger = loggingService.get_logger();

    PerfTestTopology topology;
    topology.logger = logger;
    topology.activate("parse");
    topology.log_status();

    RoundRobinScheduler scheduler(topology);
    scheduler.logger = loggingService.get_logger("RoundRobinScheduler");

    ThreadManager threadManager(scheduler);
    threadManager.logger = loggingService.get_logger("ThreadManager");
    threadManager.run(5);


//    while (topology.is_active()) {
    for (int i=0; i<100; ++i) {  // This topology will run indefinitely
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "\033[2J";
        topology.log_status();
    }

    LOG_INFO(logger) << "Stopping ..." << LOG_END;
    threadManager.stop();
    threadManager.join();
    LOG_INFO(logger) << "... Stopped." << LOG_END;
}



