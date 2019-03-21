
#include <greenfield/JLogger.h>
#include <greenfield/Topology.h>
#include <greenfield/ThreadManager.h>
#include <greenfield/Worker.h>
#include <greenfield/TopologyTestFixtures.h>

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

    Topology topology;

    auto q0 = new Queue<int>;
    auto q1 = new Queue<double>;
    auto q2 = new Queue<double>;

    topology.addQueue(q0);
    topology.addQueue(q1);
    topology.addQueue(q2);

    topology.addArrow("emit_rand_ints", new RandIntSourceArrow(q0));
    topology.addArrow("multiply_by_two", new MultByTwoArrow(q0, q1));
    topology.addArrow("subtract_one", new SubOneArrow(q1, q2));
    topology.addArrow("sum_everything", new SumArrow<double>(q2));

    RoundRobinScheduler scheduler(topology);
    scheduler.logger = loggingService.get_logger("RoundRobinScheduler");
    Scheduler::Report report;

    ThreadManager threadManager(scheduler);
    threadManager.logger = loggingService.get_logger("ThreadManager");
    threadManager.run(5);
    threadManager.join();
}



