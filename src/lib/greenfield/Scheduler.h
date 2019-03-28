#pragma once

#include <map>
#include <queue>
#include <mutex>

#include <greenfield/Topology.h>
#include "Logger.h"


namespace greenfield {

    class Scheduler {

    public:

        struct Report {

            /// Information collected by a Worker during the course of execution,
            /// which helps the Scheduler make its decision re what to do next

            uint32_t worker_id = 0;
            Arrow *assignment = nullptr;
            uint64_t event_count = 0;
            double latency_sum = 0.0;
            StreamStatus last_result = StreamStatus::ComeBackLater;
        };


    public:
        /// Tell someone (usually a Worker) what arrow they should execute next.
        /// If no assignments are available, return nullptr.
        /// This is called by all of the workers and updates Metrics and Topology,
        /// so it must be kept threadsafe
        virtual Arrow *next_assignment(const Report &report) = 0;

        Logger logger; // Control verbosity of scheduler without knowing which impl is in use
    };



    class FixedScheduler : public Scheduler {

    private:
        Topology& _topology;
        std::vector<Arrow*> _assignments;
        std::mutex _mutex;

    public:
        FixedScheduler(Topology& topology, std::map<std::string, int> nthreads);
        bool rebalance(std::string, std::string, int);
        Arrow* next_assignment(const Report& report);
    };



    class RoundRobinScheduler : public Scheduler {

    private:
        Topology& _topology;
        std::map<std::string, Arrow*>::iterator _assignment;
        std::mutex _mutex;

        Arrow* next_assignment();

    public:
        RoundRobinScheduler(Topology& topology);
        Arrow* next_assignment(const Report& report);
    };
}




