#pragma once

#include <thread>
#include <cmath>
#include <iostream>
#include <greenfield/Logger.h>
#include <greenfield/Scheduler.h>

namespace greenfield {

    class Worker {
        /// Designed so that the Worker checks in with the manager on his own terms;
        /// i.e. the manager will never update the worker's assignment without
        /// him knowing. This eliminates a whole lot of synchronization since we can assume
        /// that the Worker's internal state won't be updated by another thread.
        /// The manager is still responsible for changing the worker's
        /// assignment and checkin period.

    private:
        std::thread* _thread;
        Scheduler & _scheduler;

    public:
        const uint32_t worker_id;
        double checkin_time = 1000000;   // TODO: Determine who should set this and when

        bool shutdown_requested = false;   // For communicating with ThreadManager
        bool shutdown_achieved = false;
        Logger _logger;

        Scheduler::Report report;  // For communicating with Scheduler
        Arrow* assignment = nullptr;


        Worker(uint32_t id, Scheduler & scheduler) :
            _scheduler(scheduler), worker_id(id) {

            report.worker_id = worker_id;
            report.last_result = StreamStatus::KeepGoing;

            _thread = new std::thread(&Worker::loop, this);
            LOG_DEBUG(_logger) << "Worker " << worker_id << " constructed." << LOG_END;
        }

        // If we copy or move the Worker, the underlying std::thread will be left with a
        // dangling pointer back to `this`. So we forbid copying, assigning, and moving.

        Worker(const Worker & other) = delete;
        Worker(Worker&& other) = delete;
        Worker& operator=(const Worker & other) = delete;


        ~Worker() {
            // We have to be careful here because this Worker might be being concurrently
            // read/modified by Worker.thread. Join with thread before doing anything else.

            LOG_DEBUG(_logger) << "Worker " << worker_id << " destruction has begun." << LOG_END;
            if (_thread == nullptr) {
                LOG_ERROR(_logger) << "Worker " << worker_id << " thread is null. This means we have a problem!" << LOG_END;
            }
            else {
                LOG_DEBUG(_logger) << "Worker " << worker_id << " joining thread." << LOG_END;
                _thread->join();            // Does this throw? Can we guarantee this terminates?
                delete _thread;             // Should Worker lifetime really match thread lifetime?
                _thread = nullptr;
            }
            LOG_DEBUG(_logger) << "Worker " << worker_id << " destruction has completed." << LOG_END;
        }


        void loop() {

            LOG_DEBUG(_logger) << "Worker " << worker_id << " has entered loop()." << LOG_END;
            report.assignment = nullptr;
            report.last_result = StreamStatus::ComeBackLater;

            while (!shutdown_requested) {

                LOG_TRACE(_logger) << "Worker " << worker_id << " is checking in" << LOG_END;
                assignment = _scheduler.next_assignment(report);

                report.assignment = assignment;
                report.last_result = StreamStatus::KeepGoing;
                report.latency_sum = 0;
                report.event_count = 0;

                if (assignment == nullptr) {
                    LOG_TRACE(_logger) << "Worker " << worker_id
                                       << " shutting down due to lack of assignments" << LOG_END;
                    shutdown_requested = true;
                }
                else {
                    while (report.last_result == StreamStatus::KeepGoing &&
                           report.latency_sum < checkin_time &&
                           !shutdown_requested) {

                        LOG_TRACE(_logger) << "Worker " << worker_id << " is executing "
                                           << report.assignment->get_name() << LOG_END;
                        auto start_time = std::chrono::steady_clock::now();
                        report.last_result = assignment->execute();
                        auto stop_time = std::chrono::steady_clock::now();
                        report.latency_sum += (stop_time - start_time).count();
                        ++report.event_count;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            if (report.assignment != nullptr) {
                // If we were shut down by the ThreadManager and never had a chance to
                // check back in with the scheduler, we should update our arrow's thread count
                // ourselves (otherwise the Scheduler would do it). TODO: Make this cleaner
                report.assignment->update_thread_count(-1);
            }
            LOG_DEBUG(_logger) << "Worker " << worker_id << " is exiting loop()" << LOG_END;
            shutdown_achieved = true;
        }
    };
}



