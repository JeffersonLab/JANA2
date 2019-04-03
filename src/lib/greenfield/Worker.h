#pragma once

#include <thread>
#include <cmath>
#include <iostream>
#include <greenfield/Logger.h>
#include <greenfield/Scheduler.h>

namespace greenfield {

    using clock_t = std::chrono::steady_clock;

    class Worker {
        /// Designed so that the Worker checks in with the scheduler on his own terms;
        /// i.e. nobody will update the worker's assignment externally. This eliminates
        /// a whole lot of synchronization since we can assume
        /// that the Worker's internal state won't be updated by another thread.

    private:
        std::thread* _thread;
        Scheduler & _scheduler;
        std::mutex _mutex;  // For controlling access to total_loop_time, total_scheduler_time

    public:
        const uint32_t worker_id;
        uint32_t backoff_tries = 4;
        duration_t checkin_time = std::chrono::milliseconds(500);
        duration_t initial_backoff_duration = std::chrono::microseconds(1000);
        duration_t total_loop_time = duration_t::zero();
        duration_t total_scheduler_time = duration_t::zero();

        bool shutdown_requested = false;   // For communicating with ThreadManager
        bool shutdown_achieved = false;
        Logger _logger;

        Arrow* assignment = nullptr;


        Worker(uint32_t id, Scheduler & scheduler) :
            _scheduler(scheduler), worker_id(id) {

            _thread = new std::thread(&Worker::loop, this);
            if (serviceLocator != nullptr) {
                LOG_DEBUG(_logger) << "Worker " << worker_id << " found parameters from serviceLocator." << LOG_END;
                auto params = serviceLocator->get<ParameterManager>();
                checkin_time = params->checkin_time;
                initial_backoff_duration = params->backoff_time;
                backoff_tries = params->backoff_tries;
            }
            _logger = LoggingService::logger("Worker");
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
                auto start_time = clock_t::now();

                assignment = _scheduler.next_assignment(worker_id, assignment, last_result);

                auto scheduler_duration = clock_t::now() - start_time;
                auto loop_duration = duration_t::zero();
                    // We want to execute arrow at least once even if scheduler
                    // took longer than checkin_time

                if (assignment == nullptr) {
                    LOG_TRACE(_logger) << "Worker " << worker_id
                                       << " idling due to lack of assignments" << LOG_END;
                    std::this_thread::sleep_for(checkin_time);
                    loop_duration = clock_t::now() - start_time;
                    // Loop duration includes idle time, until we decide to track idle time separately
                }
                else {

                    uint32_t current_tries = 0;
                    auto backoff_duration = initial_backoff_duration;

                    while (current_tries <= backoff_tries &&
                           (last_result == StreamStatus::KeepGoing || last_result == StreamStatus::ComeBackLater) &&
                           !shutdown_requested &&
                           loop_duration<checkin_time ) {

                        LOG_TRACE(_logger) << "Worker " << worker_id << " is executing "
                                           << assignment->get_name() << LOG_END;
                        last_result = assignment->execute();
                        if (last_result == StreamStatus::KeepGoing) {
                            LOG_TRACE(_logger) << "Worker " << worker_id << " succeeded at "
                                               << assignment->get_name() << LOG_END;
                            current_tries = 0;
                            backoff_duration = initial_backoff_duration;
                        }
                        else {
                            current_tries++;
                            if (backoff_tries != 0) {
                                backoff_duration *= 2;
                                LOG_TRACE(_logger) << "Worker " << worker_id << " backing off with "
                                                   << assignment->get_name() << ", tries = " << current_tries
                                                   << LOG_END;

                                std::this_thread::sleep_for(backoff_duration);
                                // TODO: Randomize backoff duration?
                            }
                        }
                        loop_duration = clock_t::now() - start_time;
                    }
                }
                _mutex.lock();
                total_loop_time += loop_duration;
                total_scheduler_time += scheduler_duration;
                _mutex.unlock();
            }

            _scheduler.last_assignment(worker_id, assignment, last_result);

            LOG_DEBUG(_logger) << "Worker " << worker_id << " is exiting loop()" << LOG_END;
            shutdown_achieved = true;
        }

    };
}



