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
        std::mutex _mutex;  // For controlling access to total_loop_time, total_scheduler_time

    public:
        const uint32_t worker_id;
        std::chrono::duration<long,std::ratio<1,1000>> checkin_time = std::chrono::milliseconds(500);
        std::chrono::steady_clock::duration total_loop_time;
        std::chrono::steady_clock::duration total_scheduler_time;

        bool shutdown_requested = false;   // For communicating with ThreadManager
        bool shutdown_achieved = false;
        Logger _logger;

        Arrow* assignment = nullptr;


        Worker(uint32_t id, Scheduler & scheduler) :
            _scheduler(scheduler), worker_id(id) {

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
                LOG_ERROR(_logger) << "Worker " << worker_id << " thread is null. This means we deleted twice somehow!" << LOG_END;
            }
            else {
                LOG_DEBUG(_logger) << "Worker " << worker_id << " joining thread." << LOG_END;
                _thread->join();            // Does this throw? Can we guarantee this terminates?
                delete _thread;             // Should Worker lifetime really match thread lifetime?
                _thread = nullptr;          // Catch and log error if somehow we try to delete twice
            }
            LOG_DEBUG(_logger) << "Worker " << worker_id << " destruction has completed." << LOG_END;
        }


        void loop() {

            LOG_DEBUG(_logger) << "Worker " << worker_id << " has entered loop()." << LOG_END;
            assignment = nullptr;
            StreamStatus last_result = StreamStatus::ComeBackLater;

            while (!shutdown_requested) {

                LOG_TRACE(_logger) << "Worker " << worker_id << " is checking in" << LOG_END;
                auto start_time = std::chrono::steady_clock::now();

                assignment = _scheduler.next_assignment(worker_id, assignment, last_result);
                last_result = StreamStatus::KeepGoing;

                auto scheduler_duration = std::chrono::steady_clock::now() - start_time;
                auto loop_duration = std::chrono::steady_clock::now() - start_time;


                if (assignment == nullptr) {
                    LOG_TRACE(_logger) << "Worker " << worker_id
                                       << " idling due to lack of assignments" << LOG_END;
                    std::this_thread::sleep_for(checkin_time);
                }
                else {
                    while (last_result == StreamStatus::KeepGoing &&
                           !shutdown_requested &&
                           loop_duration<checkin_time ) {

                        LOG_TRACE(_logger) << "Worker " << worker_id << " is executing "
                                           << assignment->get_name() << LOG_END;
                        last_result = assignment->execute();
                        loop_duration = std::chrono::steady_clock::now() - start_time;
                    }
                }
                _mutex.lock();
                total_loop_time += std::chrono::steady_clock::now() - start_time;
                total_scheduler_time += scheduler_duration;
                _mutex.unlock();
            }

            _scheduler.last_assignment(worker_id, assignment, last_result);

            LOG_DEBUG(_logger) << "Worker " << worker_id << " is exiting loop()" << LOG_END;
            shutdown_achieved = true;
        }

    };
}



