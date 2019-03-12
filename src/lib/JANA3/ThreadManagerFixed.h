
#pragma once

#include <JANA3/ThreadManager.h>


namespace futurejana {


  struct Worker {

    Arrow* _arrow;
    Topology& _topology;

    atomic<bool> _finished(false);
    int _event_count = 0;
    int _latency_idx = 0;
    float[10] _latencies {0,0,0,0,0,0,0,0,0,0};

    Worker(Topology& t, Arrow* a) : _topology(t), _arrow(a) {}

    void loop() {
      while (!_finished) {
        // TODO: start clock
        _arrow->execute(_topology);
        // stop clock
        // _latencies[latency_idx] = end_time - start_time;
        (++latency_idx) %= 10;
        ++_event_count;
      }
    }
  }


  class ThreadManager_Fixed {
    /// ThreadManager_Fixed makes no attempt to rebalance. 
    /// Every arrow is assigned one thread unless the user 
    /// specifies otherwise.

    map<Arrow*, int> _thread_counts;

    vector<Worker> _workers; // The thread state
    // vector<thread> _threads; // TODO: The threads themselves

  public:

    ThreadManager(Topology& topology) {

      for (Arrow& arrow : topology.arrows) {
        _thread_counts[&arrow] = 1;
      }

    }

    void set_threads(Arrow* arrow, int nthreads) {
      _thread_counts[arrow] = nthreads;
    }

    void run() {
      // TODO: Create correct number of workers for each arrow
      // TODO: Launch threads for each worker
    }

    void soft_stop() {

      for (Worker& worker : _workers) {
        _worker._finished = true;
      }
      // TODO: join all threads as well?

    }

    void hard_stop() {
      // TODO: for (thread : _threads) thread.kill();
    }


    vector<ThreadStatus> status() {

      map<Arrow*, ThreadStatus> acc;
      vector<ThreadStatus> results;

      for (Worker& worker : _workers) {

        ThreadStatus& ts = acc[worker.arrow];
        ts.arrow_name = typeid(worker.arrow).name();
        ts.nthreads += 1;
        ts.events_completed += worker._event_count;

        float worker_latency = 0;
        int nsamples = min(10, ts.events_completed);
        for (int i=0; i<nsamples; ++i) {
          worker_latency += worker._latencies[i];
        }
        worker_latency /= nsamples;
        ts.latency_avg += worker_latency;
      }

      for (auto & item : acc) {
        item.second.latency_avg /= item.second.nthreads;
        results.push_back(item.second);
      }

      return results;
    }
  }


}

