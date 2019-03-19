
#include <thread>
#include <cmath>


namespace greenfield {

    class Worker {
        /// Designed so that the Worker checks in with the manager on his own terms;
        /// i.e. the manager will never update the worker's assignment without
        /// him knowing. This eliminates a whole lot of synchronization since we can assume
        /// that the Worker's internal state won't be updated by another thread.
        /// The manager is still responsible for changing the worker's
        /// assignment and checkin period.


    private:
        ThreadManager * manager;
        std::thread* thread;

    public:
        // Updated by scheduler
        Arrow * assignment; // nullptr => idle state
        double checkin_time;
        bool shutdown_requested;

        // Updated by worker
        int event_count;
        double latency_sum;
        SchedulerHint last_result;
        bool shutdown_achieved;


        Worker(ThreadManager * manager, Arrow * assignment, double checkin_time) :
            manager(manager), assignment(assignment), checkin_time(checkin_time) {

            assert(manager != nullptr);
            assert(assignment != nullptr);

            shutdown_requested = false;
            shutdown_achieved = false;

            thread = new std::thread(&Worker::loop, this);
        }


        ~Worker() {
            shutdown_requested = true; // Probably a race condition here
            thread->join();            // Does this throw? Can we guarantee this terminates?
            delete thread;             // Should Worker lifetime really match thread lifetime?
        }


        void loop() {

            while (!shutdown_requested) {
                latency_sum = 0;
                event_count = 0;
                last_result = SchedulerHint::KeepGoing;

                if (assignment == nullptr) {
                    std::chrono::duration<double, std::ratio<1>> checkin_secs(checkin_time);
                    std::this_thread::sleep_for(checkin_secs);
                }
                else {
                    while (last_result == SchedulerHint::KeepGoing &&
                           latency_sum < checkin_time &&
                           !shutdown_requested) {

                        auto start_time = std::chrono::steady_clock::now();
                        last_result = assignment->execute();
                        auto stop_time = std::chrono::steady_clock::now();
                        latency_sum += (stop_time - start_time).count();
                        ++event_count;
                    }
                }
                manager->checkin(this);
            }
            shutdown_achieved = true;
        }
    };
}
