
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

void log_scheduler_decision(uint32_t worker, Arrow* before, SchedulerHint feedback, Arrow* after) {
    auto logger = make_shared<JLogger>();
    LOG_INFO(logger) << "Scheduler: (" << worker << ", "
                     << ((before == nullptr) ? "nullptr" : before->get_name())
                     << ", " << to_string(feedback) << ") => "
                     << ((after == nullptr) ? "nullptr" : after->get_name())
                     << LOG_END;

}

int main() {
    auto logger = make_shared<JLogger>();

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
    Scheduler::Report report;

    // If we only have one worker and it always gets comebacklater, we do round-robin
    report.last_result = SchedulerHint::ComeBackLater;
    for (int i=0; i<10; ++i) {
        Arrow* assignment = scheduler.next_assignment(report);
        log_scheduler_decision(report.worker_id, report.assignment, report.last_result, assignment);
        report.assignment = assignment;
    }

    LOG_INFO(logger) << "------------------------------" << LOG_END;
    // If we have a bunch of workers and they start off with (nullptr, comebacklater),
    // only one gets each sequential thread and the rest get the parallel threads, round-robin
    report.last_result = SchedulerHint::ComeBackLater;
    for (int i=0; i<10; ++i) {
        report.assignment = nullptr;
        Arrow* assignment = scheduler.next_assignment(report);
        log_scheduler_decision(report.worker_id, report.assignment, report.last_result, assignment);
        report.assignment = assignment;
    }

    // Suppose all the arrows are assigned.
    // assert that once scheduler receives a sequential arrow back,
    // it will offer it back out, but only once
    LOG_INFO(logger) << "------------------------------" << LOG_END;
    report.assignment = topology.arrows["sum_everything"];
    report.last_result = SchedulerHint::ComeBackLater;

    for (int i=0; i<8; ++i) {
        Arrow* assignment = scheduler.next_assignment(report);
        log_scheduler_decision(report.worker_id, report.assignment, report.last_result, assignment);
        report.assignment = nullptr;
        // assert that scheduler does something sensible with sum_everything
    }

    // Suppose one arrow is successful. Then the scheduler should respect that
    LOG_INFO(logger) << "------------------------------" << LOG_END;
    report.last_result = SchedulerHint::KeepGoing;
    report.assignment = topology.arrows["subtract_one"];

    for (int i=0; i<4; ++i) {
        Arrow* assignment = scheduler.next_assignment(report);
        log_scheduler_decision(report.worker_id, report.assignment, report.last_result, assignment);
        report.assignment = assignment;
        // assert that scheduler keeps going with this assignment
    }


    ThreadManager threadManager(scheduler);
    threadManager.run(5);
    threadManager.join();
}



