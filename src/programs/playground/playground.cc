
#include <greenfield/JLogger.h>
#include <greenfield/Topology.h>
#include <greenfield/ThreadManager.h>
#include <greenfield/Worker.h>
#include <greenfield/ExampleComponents.h>
#include <greenfield/LinearTopologyBuilder.h>

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
    loggingService.set_level("RoundRobinScheduler", JLogLevel::TRACE);
    loggingService.set_level("ThreadManager", JLogLevel::TRACE);
    auto logger = loggingService.get_logger();


    LinearTopologyBuilder b;
    RandIntSource source;
    MultByTwoProcessor p1;
    SubOneProcessor p2;
    SumSink<double> sink;

    b.addSource("emit_rand_ints", source);
    b.addProcessor("multiply_by_two", p1);
    b.addProcessor("subtract_one", p2);
    b.addSink("sum_everything", sink);

    auto topology = b.get();

    RoundRobinScheduler scheduler(topology);
    scheduler.logger = loggingService.get_logger("RoundRobinScheduler");
    Scheduler::Report report;

    ThreadManager threadManager(scheduler);
    threadManager.logger = loggingService.get_logger("ThreadManager");
    threadManager.run(5);
    threadManager.join();
}



