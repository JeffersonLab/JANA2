
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
    auto logger = make_shared<JLogger>();

    Topology topology;

    auto q0 = new Queue<int>;
    auto q1 = new Queue<double>;

    topology.addQueue(q0);
    topology.addQueue(q1);

    topology.addArrow("emit_rand_ints", new RandIntSourceArrow(q0));
    topology.addArrow("multiply_by_two", new MultByTwoArrow(q0, q1));
    topology.addArrow("sum_everything", new SumArrow<double>(q1));

    RoundRobinScheduler scheduler(topology);
    ThreadManager threadManager(scheduler);
    LOG_INFO(logger) << "Running thread manager" << LOG_END;
    auto result = threadManager.run(22);
    LOG_INFO(logger) << "result = " << stringify(result) << LOG_END;

}



