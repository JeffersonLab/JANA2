#pragma once

using std::string;

namespace futurejana {


  struct ThreadStatus {

    /// A view of how many threads are running each arrow,
    /// and how they are performing wrt latency and throughput.
    /// This is liable to come from different internal
    /// representations for different ThreadManagers.
    /// Queue status should be read from the Topology instead, 
    /// because it does NOT depend on the ThreadManager implementation.

    string arrow_name;
    int nthreads;
    int events_completed;
    double latency_avg;
  }



  struct IThreadManager {

    /// ThreadManager launches a team of threads and assigns them arrows,
    /// implementing some kind of rebalancing strategy. 
    /// This is designed with the aim of separating concerns
    /// so that that many different rebalancing strategies
    /// (or even multithreading technologies) can be transparently
    /// applied to the same Topology.


    virtual void run() = 0;
    /// The Topology contains all of the information needed to run
    /// the system. 

    virtual void soft_stop() = 0;
    /// soft_stop should leave the Topology in a well-defined state
    /// (i.e. no messages get lost) so that it can be continued later,
    /// possibly with a different ThreadManager.

    virtual void hard_stop() = 0;
    /// hard_stop should kill all threads immediately without worrying
    /// about data loss.

    virtual vector<ThreadStatus> status() = 0;
    /// status() provides a view of thread performance
    /// See ThreadStatus for more details.
  }

}


