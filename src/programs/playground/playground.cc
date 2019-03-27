
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
    SumSink<double> sink;

    b.addSource("emit_rand_ints", source);
    b.addProcessor<MultByTwoProcessor>("multiply_by_two");
    b.addProcessor<SubOneProcessor>("subtract_one");
    b.addSink("sum_everything", sink);

    auto topology = b.get();
    topology.logger = logger;
    source.logger = logger;
    topology.activate("emit_rand_ints");
    topology.log_status();

    RoundRobinScheduler scheduler(topology);
    scheduler.logger = loggingService.get_logger("RoundRobinScheduler");

    ThreadManager threadManager(scheduler);
    threadManager.logger = loggingService.get_logger("ThreadManager");
    threadManager.run(5);
    threadManager.join();

    topology.log_status();
}



