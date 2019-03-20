#pragma once

#include <map>
#include <queue>
#include <mutex>

#include <greenfield/Topology.h>


namespace greenfield {

    class Scheduler {

    public:

        struct Report {

            /// Information collected by a Worker during the course of execution,
            /// which helps the Scheduler make its decision re what to do next

            int worker_id;
            Arrow* assignment;
            int event_count;
            double latency_sum;
            SchedulerHint last_result;
        };


        struct Metric {

            /// A view of how many threads are running each arrow,
            /// and how they are performing wrt latency and throughput.
            /// Queue status should be read from the Topology instead,
            /// because it does NOT depend on the ThreadManager implementation.

            std::string arrow_name;
            int nthreads;
            bool is_parallel;
            bool is_finished;
            int events_completed;
            double short_term_avg_latency;
            double long_term_avg_latency;
        };


    protected:
        std::map<Arrow*, Metric> _metrics;


    public:
        /// Tell someone (usually a Worker) what arrow they should execute next.
        /// If no assignments are available, return nullptr.
        /// This is called by all of the workers and updates Metrics and Topology,
        /// so it must be kept threadsafe
        virtual Arrow* next_assignment(const Report& report) = 0;


        /// Provide a copy of performance statistics for all Arrows in the Topology
        std::vector<Metric> get_metrics() {

            std::vector<Metric> metrics(_metrics.size());

            for (auto & pair : _metrics) {
                auto metric = pair.second;
                metrics.push_back(metric);  // This copies the metric
            }

            for (auto & metric : metrics) {
                // Convert from running total to average
                metric.long_term_avg_latency /= metric.events_completed;
            }
            return metrics;
        }


        /// Given a report, update the metrics information
        // TODO: Currently, the caller needs to be synchronized, which isn't great.
        // TODO: Can report.last_result() ever diverge from arrow->is_finished() ??
        void updateMetrics(const Report& report) {

            Arrow* arrow = report.assignment;
            Metric& metric = _metrics[arrow];

            metric.events_completed += report.event_count;
            metric.long_term_avg_latency += report.latency_sum;
            metric.short_term_avg_latency = report.latency_sum / report.event_count;

            if (report.last_result == SchedulerHint::Finished) {
                metric.is_finished = true;
            }
        }
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
        std::queue<Arrow*> _assignments;
        std::mutex _mutex;

        Arrow* next_assignment();

    public:
        RoundRobinScheduler(Topology& topology);
        Arrow* next_assignment(const Report& report);
    };
}




