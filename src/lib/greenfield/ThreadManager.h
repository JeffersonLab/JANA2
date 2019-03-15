#pragma once

#include <list>

#include <greenfield/Topology.h>


using std::string;

namespace greenfield {


    struct ArrowStatus {

        /// A view of how many threads are running each arrow,
        /// and how they are performing wrt latency and throughput.
        /// Queue status should be read from the Topology instead,
        /// because it does NOT depend on the ThreadManager implementation.

        string arrow_name;
        int nthreads;
        bool is_parallel;
        bool is_finished;
        int events_completed;
        double short_term_avg_latency;
        double long_term_avg_latency;
    };

    struct Worker {
        /// Designed so that the Worker checks in with the manager on his own terms;
        /// i.e. the manager will never update the worker's assignment without
        /// him knowing. This eliminates a whole lot of synchronization since we can assume
        /// that . The manager is still responsible for changing the worker's
        /// assignment and checkin period.

        ThreadManager* manager;
        Arrow* assignment;

        int event_count = 0;
        double latency_sum = 0.0;
        double checkin_time = 100;
        bool shutdown_requested = false;
        bool shutdown_achieved = false;
        SchedulerHint last_result = SchedulerHint::KeepGoing;

        void loop() {

            while (!shutdown_requested) {
                while (last_result == SchedulerHint::KeepGoing && latency_sum < checkin_time) {
                    // TODO: start clock
                    last_result = assignment->execute();
                    // stop clock
                    // latency_sum += end_time - start_time;
                    ++event_count;
                }
                manager->checkin(this);
            }
        }
    };


    class ThreadManager {

        /// ThreadManager maintains a bunch of state around a Topology.
        /// A ThreadManager has exactly one Topology,
        /// (Note that this Topology need not be a connected graph)
        /// For now, the ThreadManager does not own the Topology; instead
        /// the caller does, so that the caller may inspect the topology,
        /// issue commands to the ThreadManager (e.g. move one worker from
        /// arrow A to arrow B), pause execution, and possibly even resume
        /// execution using a different ThreadManager.
        /// This may change.

        /// ThreadManager launches a team of threads and assigns them arrows,
        /// implementing some kind of rebalancing strategy.
        /// This is designed with the aim of separating concerns
        /// so that that many different rebalancing strategies
        /// (or even multithreading technologies) can be transparently
        /// applied to the same Topology.

        int _nthreads;
        Topology& _topology;
        std::list<Arrow*> _pending_assignments;
        std::map<Arrow*, ArrowStatus> _arrow_statuses;
        std::vector<Worker> _workers;
        std::mutex _mutex; // Controls access to checkin()

    public:

        ThreadManager(Topology& topology, int nthreads) : _topology(topology), _nthreads(nthreads) {
            for (Arrow* arrow : topology.arrows) {
                //    Add to pending assignments
                //    Add to arrow statuses
            }
        }

        void run() {
            // Create workers
            // For each worker, have them check in with manager to get first assignment
            // Spawn threads for each worker
        }

        // Tools for manually interacting with a paused topology
        SchedulerHint step(std::string arrow_name) {
            // Find arrow with corresponding name
            // Call arrow.execute
            // Maybe we want some kind of Queue.peek() as well?
        }

        // Tools for manually interacting with a running topology
        void spawn_worker(std::string arrow_name) {
            // Find arrow with corresponding name.
            // Look for a worker who has been told to stop and who has actually stopped.
            // Spawn a new thread for this worker, giving him that assignment

        }

        void kill_worker(std::string arrow_name) {
            // Find the first worker assigned to this arrow who
            // Set worker.is_finished = true; (this requires synchronization :/ )
        }

        void pause() {
            /// pause should leave the Topology in a well-defined state
            /// (i.e. no messages get lost) so that it can be continued later,
            /// possibly with a different ThreadManager.

            for (Worker& worker : _workers) {
                worker.shutdown_requested = true;  // This requires synchronization
            }
        }

        void kill() {
            /// kill should kill all threads immediately without worrying
            /// about data loss.
        }

        std::vector<ArrowStatus> get_status() {
            /// This is doing a complete copy of _arrow_statuses because
            /// ThreadManager is also using that information for scheduling,
            /// so we don't want the user to be able to modify it
            return _arrow_statuses;
        }

        void checkin(Worker* worker) {


        }

    };

}


