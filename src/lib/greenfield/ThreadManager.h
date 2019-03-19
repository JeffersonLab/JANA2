#pragma once

#include <list>
#include <vector>
#include <map>
#include <mutex>

#include <greenfield/Topology.h>

using std::string;
using std::vector;
using std::list;
using std::map;
using std::mutex;

namespace greenfield {

    class Worker;

    class ThreadManager {

        /// ThreadManager assigns a Topology to a team of Worker threads.
        /// A ThreadManager has exactly one Topology for its lifetime,
        /// which it does not own. (The caller may want to inspect the
        /// internals of the Topology, and it may be paused and re-run
        /// on a different ThreadManager.

        /// ThreadManager maintains state for tracking performance and
        /// rebalancing work as needed. It completely encapsulates (and
        /// thereby owns) its Worker team.

        /// ThreadManager provides the user with a high-level interface
        /// for controlling the thread team and examining its performance.
        /// The user may start, stop, kill, scale, and rebalance

        /// ThreadManager provides Workers with a checkin() method that
        /// updates their assignments according to some rebalancing algorithm.
        /// This algorithm may eventually be made into a Strategy or Policy.


        // ThreadManager-specific types are defined here in order to keep namespaces clean

        enum class Response { Success, NotRunning, AlreadyRunning, TooManyWorkers,
            TooFewWorkers, ArrowNotFound, ArrowFinished, InProgress };

        enum class Status { Idle, Running, Stopping };

        struct Metric {

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


    private:

        Topology& _topology;
        Status _status;             // Written by master thread, read by all threads during checkin(),
        vector<Worker> _workers;    // After construction, only accessed from within own thread

        list<Arrow*> _upcoming_assignments;  // Written to by workers, read by master
        map<Arrow*, Metric> _metrics;        // Written to by workers, read by master

        mutex _mutex;


    public:

        // Public API

        /// A ThreadManager has exactly one Topology in its lifetime, which it does _not_ own.
        ThreadManager(Topology& topology);

        /// The ThreadManager is idle until start() is called. start() is nonblocking.
        /// This can fail: AlreadyRunning
        Response run(int nthreads);

        /// Pause a running Topology, leaving it in a well-defined state
        /// that can be restarted and experiences no data loss. pause is nonblocking.
        /// There are three outcomes: Success | InProgress | NotRunning
        Response stop();

        /// Block until ThreadManager is back at Status::Idle and all Worker threads have joined.
        /// This will wait until all Queues are empty and all Arrows are finished unless
        /// stop() is called first. Returns NotRunning if already joined, otherwise Success.
        Response join();

        /// Manually scale up by adding more workers to a given arrow.
        /// This can fail: ArrowNotFound | TooManyWorkers | TooFewWorkers | ArrowFinished
        Response scale(string arrow_name, int delta);

        /// Manually rebalance by moving workers from one arrow to another
        /// This can fail: ArrowNotFound | TooFewWorkers | TooManyWorkers | ArrowFinished
        Response rebalance(string prev_arrow_name, string next_arrow_name, int delta);

        /// Returns the current state of the TopologyManager
        Status get_status();

        /// Provide a copy of performance statistics for all Arrows in the Topology
        vector<Metric> get_metrics();


        // For internal use only

        /// Receive performance information from Worker, update _arrow_statuses,
        /// and give the Worker a new assignment and checkin_time.
        /// This is meant to be called from Worker::loop()
        void checkin(Worker* worker);


    private:

        /// Figure out the next assignment.
        Arrow * next_assignment();

    };

}


