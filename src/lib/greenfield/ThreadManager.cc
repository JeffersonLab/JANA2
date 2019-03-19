
#include <list>

#include <greenfield/ThreadManager.h>
#include <greenfield/Worker.h>

using std::string;

namespace greenfield {


    ThreadManager::ThreadManager(Topology& topology) :
        _topology(topology), _status(Status::Idle) {};


    ThreadManager::Status ThreadManager::get_status() {
        return _status;
    }


    ThreadManager::Response ThreadManager::run(int nthreads) {

        if (_status != Status::Idle) {
            return Response::AlreadyRunning;
        }
        _status = Status::Running;

        for (int i=0; i<nthreads; ++i) {

            Arrow * assignment = next_assignment();
            if (assignment == nullptr) { break; }  // No more arrows to assign threads to!
            _workers.emplace_back(this, assignment, 10);
        }
        return Response::Success;
    }


    ThreadManager::Response ThreadManager::stop() {
        if (_status == Status::Idle) {
            return Response::NotRunning;
        }
        _status = Status::Stopping;
        bool all_stopped = true;
        for (Worker & worker : _workers) {
            worker.shutdown_requested = true;
            all_stopped &= worker.shutdown_achieved;
        }
        if (all_stopped) {
            return Response::Success;
        }
        else {
            return Response::InProgress;
        }
    }


    ThreadManager::Response ThreadManager::join() {
        if (_status == Status::Idle) {
            return Response::NotRunning;
        }
        // We don't set worker.shutdown_requested, so the threads won't stop
        // until they are told their input queues are finished.
        _workers.clear();  // Destroying workers joins their respective threads.
        _status = Status::Idle;
        return Response::Success;
    }


    ThreadManager::Response ThreadManager::scale(string arrow_name, int delta) {
        if (_status != Status::Running) {
            return Response::NotRunning;
        }
        return Response::Success;
    }


    ThreadManager::Response ThreadManager::rebalance(string prev_arrow_name,
                                                     string next_arrow_name,
                                                     int delta) {
        return Response::Success;
    }


    vector<ThreadManager::Metric> ThreadManager::get_metrics() {

        vector<Metric> metrics(_metrics.size());

        _mutex.lock();
        for (auto & pair : _metrics) {
            auto metric = pair.second;
            metrics.push_back(metric);  // This copies the metric
        }
        _mutex.unlock();

        for (auto & metric : metrics) {
            // Convert from running total to average
            metric.long_term_avg_latency /= metric.events_completed;
        }
        return metrics;
    }


    void ThreadManager::checkin(Worker* worker) {

        _mutex.lock();

        auto arrow = worker->assignment;
        Metric & metric = _metrics[arrow];
        metric.events_completed += worker->event_count;
        metric.long_term_avg_latency += worker->latency_sum;
        metric.short_term_avg_latency = worker->latency_sum / worker->event_count;

        if (worker->last_result == SchedulerHint::Finished) {
            metric.is_finished = true;
        }
        if (worker->last_result != SchedulerHint::KeepGoing) {
            worker->assignment = next_assignment();
        }
        if (worker->assignment != arrow) {
            metric.nthreads--;
        }
        if (_status == Status::Stopping) {
            worker->shutdown_requested = true;
        }

        _mutex.unlock();
    };

    Arrow* ThreadManager::next_assignment() {
        while (_upcoming_assignments.size() > 0) {
            Arrow* arrow = _upcoming_assignments.front();
            _upcoming_assignments.pop_front();
            if (arrow->is_finished()) {
                // Removes arrow from the queue completely and tries the next one
                continue;
            }
            else if (!arrow->is_parallel() && _metrics[arrow].nthreads != 0) {
                // Arrow is sequential and is already assigned to a thread,
                // so we send it to the back of the queue and try again
                _upcoming_assignments.push_back(arrow);
                continue;
            }
            else {
                // Arrow is either parallel or sequential and not assigned to any threads
                // We send it to the back of the queue and also return it
                _upcoming_assignments.push_back(arrow);
                return arrow;
            }
        }
        return nullptr;  // null represents 'no viable arrows found'
    }
}



