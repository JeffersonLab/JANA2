
#include "JExecutionEngine.h"
#include <JANA/Utils/JApplicationInspector.h>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <exception>
#include <mutex>
#include <csignal>
#include <ostream>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

thread_local int jana2_worker_id = -1;
thread_local JBacktrace* jana2_worker_backtrace = nullptr;

void JExecutionEngine::Init() {
    auto params = GetApplication()->GetJParameterManager();

    params->SetDefaultParameter("jana:timeout", m_timeout_s, 
        "Max time (in seconds) JANA will wait for a thread to update its heartbeat before hard-exiting. 0 to disable timeout completely.");

    params->SetDefaultParameter("jana:warmup_timeout", m_warmup_timeout_s, 
        "Max time (in seconds) JANA will wait for 'initial' events to complete before hard-exiting.");

    params->SetDefaultParameter("jana:backoff_interval", m_backoff_ms, 
        "Max time (in seconds) JANA will wait for 'initial' events to complete before hard-exiting.");

    params->SetDefaultParameter("jana:show_ticker", m_show_ticker, "Controls whether the ticker is visible");

    params->SetDefaultParameter("jana:ticker_interval", m_ticker_ms, "Controls the ticker interval (in ms)");

    auto p = params->SetDefaultParameter("jana:status_fname", m_path_to_named_pipe,
        "Filename of named pipe for retrieving instantaneous status info");

    size_t pid = getpid();
    mkfifo(m_path_to_named_pipe.c_str(), 0666);

    LOG_WARN(GetLogger()) << "To pause processing and inspect, press Ctrl-C." << LOG_END;
    LOG_WARN(GetLogger()) << "For a clean shutdown, press Ctrl-C twice." << LOG_END;
    LOG_WARN(GetLogger()) << "For a hard shutdown, press Ctrl-C three times." << LOG_END;

    if (p->IsDefault()) {
        LOG_WARN(GetLogger()) << "For worker status information, press Ctrl-Z, or run `jana-status " << pid << "`" << LOG_END;
    }
    else {
        LOG_WARN(GetLogger()) << "For worker status information, press Ctrl-Z, or run `jana-status " << pid << " " << m_path_to_named_pipe << "`" << LOG_END;
    }


    // Not sure how I feel about putting this here yet, but I think it will at least work in both cases it needs to.
    // The reason this works is because JTopologyBuilder::create_topology() has already been called before 
    // JApplication::ProvideService<JExecutionEngine>().
    for (JArrow* arrow : m_topology->arrows) {

        arrow->initialize();

        m_arrow_states.emplace_back();
        auto& arrow_state = m_arrow_states.back();
        arrow_state.is_source = arrow->is_source();
        arrow_state.is_sink = arrow->is_sink();
        arrow_state.is_parallel = arrow->is_parallel();
        arrow_state.next_input = arrow->get_next_port_index();
    }
}


void JExecutionEngine::RunTopology() {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_runstatus == RunStatus::Failed) {
        throw JException("Cannot switch topology runstatus to Running because it is already Failed");
    }
    if (m_runstatus == RunStatus::Finished) {
        throw JException("Cannot switch topology runstatus to Running because it is already Finished");
    }
    if (m_arrow_states.size() == 0) {
        throw JException("Cannot execute an empty topology! Hint: Have you provided an event source?");
    }

    // Set start time and event count
    m_time_at_start = clock_t::now();
    m_event_count_at_start = m_event_count_at_finish;

    // Reactivate topology
    for (auto& arrow: m_arrow_states) {
        if (arrow.status == ArrowState::Status::Paused) {
            arrow.status = ArrowState::Status::Running;
        }
    }

    m_runstatus = RunStatus::Running;

    lock.unlock();
    m_condvar.notify_one();
}

void JExecutionEngine::ScaleWorkers(size_t nthreads) {
    // We both create and destroy the pool of workers here. They all sleep until they 
    // receive work from the scheduler, which won't happen until the runstatus <- {Running, 
    // Pausing, Draining} and there is a task ready to execute. This way worker creation/destruction
    // is decoupled from topology execution.

    // If we scale to zero, no workers will run. This is useful for testing, and also for using
    // an external thread team, should the need arise.

    std::unique_lock<std::mutex> lock(m_mutex);

    if (nthreads != 0 && m_arrow_states.size() == 0) {
        // We check that (nthreads != 0) because this gets called at shutdown even if the topology wasn't run
        // Remember, we want JApplication::Initialize() to succeed and JMain to shut down cleanly even when the topology is empty
        throw JException("Cannot execute an empty topology! Hint: Have you provided an event source?");
    }

    auto prev_nthreads = m_worker_states.size();

    if (prev_nthreads < nthreads) {
        // We are launching additional worker threads
        LOG_DEBUG(GetLogger()) << "Scaling up to " << nthreads << " worker threads" << LOG_END;
        for (size_t worker_id=prev_nthreads; worker_id < nthreads; ++worker_id) {
            auto worker = std::make_unique<WorkerState>();
            worker->worker_id = worker_id;
            worker->is_stop_requested = false;
            worker->cpu_id = m_topology->mapping.get_cpu_id(worker_id);
            worker->location_id = m_topology->mapping.get_loc_id(worker_id);
            worker->thread = new std::thread(&JExecutionEngine::RunWorker, this, Worker{worker_id, &worker->backtrace});
            LOG_DEBUG(GetLogger()) << "Launching worker thread " << worker_id << " on cpu=" << worker->cpu_id << ", location=" << worker->location_id << LOG_END;
            m_worker_states.push_back(std::move(worker));

            bool pin_to_cpu = (m_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
            if (pin_to_cpu) {
                JCpuInfo::PinThreadToCpu(worker->thread, worker->cpu_id);
            }
        }
    }

    else if (prev_nthreads > nthreads) {
        // We are destroying existing worker threads
        LOG_DEBUG(GetLogger()) << "Scaling down to " << nthreads << " worker threads" << LOG_END;

        // Signal to threads that they need to terminate.
        for (int worker_id=prev_nthreads-1; worker_id >= (int)nthreads; --worker_id) {
            LOG_DEBUG(GetLogger()) << "Stopping worker " << worker_id << LOG_END;
            m_worker_states[worker_id]->is_stop_requested = true;
        }
        m_condvar.notify_all(); // Wake up all threads so that they can exit the condvar wait loop
        lock.unlock();

        // We join all (eligible) threads _outside_ of the mutex
        for (int worker_id=prev_nthreads-1; worker_id >= (int) nthreads; --worker_id) {
            if (m_worker_states[worker_id]->thread != nullptr) {
                if (m_worker_states[worker_id]->is_timed_out) {
                    // Thread has timed out. Rather than non-cooperatively killing it,
                    // we relinquish ownership of it but remember that it was ours once and
                    // is still out there, somewhere, biding its time
                    m_worker_states[worker_id]->thread->detach();
                    LOG_DEBUG(GetLogger()) << "Detached worker " << worker_id << LOG_END;
                }
                else {
                    LOG_DEBUG(GetLogger()) << "Joining worker " << worker_id << LOG_END;
                    m_worker_states[worker_id]->thread->join();
                    LOG_DEBUG(GetLogger()) << "Joined worker " << worker_id << LOG_END;
                }
            }
            else {
                LOG_DEBUG(GetLogger()) << "Skipping worker " << worker_id << LOG_END;
            }
        }

        lock.lock();
        // We retake the mutex so we can safely modify m_worker_states
        for (int worker_id=prev_nthreads-1; worker_id >= (int)nthreads; --worker_id) {
            if (m_worker_states.back()->thread != nullptr) {
                delete m_worker_states.back()->thread;
            }
            m_worker_states.pop_back();
        }
    }
}

void JExecutionEngine::PauseTopology() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_runstatus != RunStatus::Running) return;
    m_runstatus = RunStatus::Pausing;
    for (auto& arrow: m_arrow_states) {
        if (arrow.status == ArrowState::Status::Running) {
            arrow.status = ArrowState::Status::Paused;
        }
    }
    LOG_WARN(GetLogger()) << "Requested pause" << LOG_END;
    lock.unlock();
    m_condvar.notify_all();
}

void JExecutionEngine::DrainTopology() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_runstatus != RunStatus::Running) return;
    m_runstatus = RunStatus::Draining;
    for (auto& arrow: m_arrow_states) {
        if (arrow.is_source) {
            if (arrow.status == ArrowState::Status::Running) {
                arrow.status = ArrowState::Status::Paused;
            }
        }
    }
    LOG_WARN(GetLogger()) << "Requested drain" << LOG_END;
    lock.unlock();
    m_condvar.notify_all();
}

void JExecutionEngine::RunSupervisor() {

    m_interrupt_status = InterruptStatus::NoInterruptsSupervised;
    size_t last_event_count = 0;
    clock_t::time_point last_measurement_time = clock_t::now();

    Perf perf;
    while (true) {

        if (m_enable_timeout && m_timeout_s > 0) {
            CheckTimeout();
        }

        if (m_print_worker_report_requested) {
            PrintWorkerReport(false);
            m_print_worker_report_requested = false;
        }

        if (m_send_worker_report_requested) {
            PrintWorkerReport(true);
            m_send_worker_report_requested = false;
        }

        perf = GetPerf();
        if ((perf.runstatus == RunStatus::Paused && m_interrupt_status != InterruptStatus::InspectRequested) || 
            perf.runstatus == RunStatus::Finished || 
            perf.runstatus == RunStatus::Failed) {
            break;
        }

        if (m_interrupt_status == InterruptStatus::InspectRequested) {
            if (perf.runstatus == RunStatus::Paused) {
                LOG_INFO(GetLogger()) << "Entering inspector" << LOG_END;
                m_enable_timeout = false;
                m_interrupt_status = InterruptStatus::InspectInProgress;
                InspectApplication(GetApplication());
                m_interrupt_status = InterruptStatus::NoInterruptsSupervised;

                // Jump back to the top of the loop so that we have fresh event count data
                last_measurement_time = clock_t::now();
                last_event_count = 0;
                continue; 
            }
            else if (perf.runstatus == RunStatus::Running) {
                PauseTopology();
            }
        }
        else if (m_interrupt_status == InterruptStatus::PauseAndQuit) {
            PauseTopology();
        }

        if (m_show_ticker) {
            auto next_measurement_time = clock_t::now();
            auto last_measurement_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(next_measurement_time - last_measurement_time).count();
            float latest_throughput_hz = (last_measurement_duration_ms == 0) ? 0 : (perf.event_count - last_event_count) * 1000.0 / last_measurement_duration_ms;
            last_measurement_time = next_measurement_time;
            last_event_count = perf.event_count;

            // Print rates
            LOG_INFO(m_logger) << "Status: " << perf.event_count << " events processed at "
                            << JTypeInfo::to_string_with_si_prefix(latest_throughput_hz) << "Hz ("
                            << JTypeInfo::to_string_with_si_prefix(perf.throughput_hz) << "Hz avg)" << LOG_END;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_ticker_ms));
    }
    LOG_INFO(GetLogger()) << "Processing paused." << LOG_END;

    if (perf.runstatus == RunStatus::Failed) {
        HandleFailures();
    }

    PrintFinalReport();
}

bool JExecutionEngine::CheckTimeout() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = clock_t::now();
    bool timeout_detected = false;
    for (auto& worker: m_worker_states) {
        auto timeout_s = (worker->is_event_warmed_up) ? m_timeout_s : m_warmup_timeout_s;
        auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(now - worker->last_checkout_time).count();
        if (duration_s > timeout_s) {
            worker->is_timed_out = true;
            timeout_detected = true;
            m_runstatus = RunStatus::Failed;
        }
    }
    return timeout_detected;
}

void JExecutionEngine::HandleFailures() {

    std::unique_lock<std::mutex> lock(m_mutex);

    // First, we log all of the failures we've found
    for (auto& worker: m_worker_states) {
        const auto& arrow_name = m_topology->arrows[worker->last_arrow_id]->get_name();
        if (worker->is_timed_out) {
            LOG_FATAL(GetLogger()) << "Timeout in worker thread " << worker->worker_id << " while executing " << arrow_name << " on event #" << worker->last_event_nr << LOG_END;
            pthread_kill(worker->thread->native_handle(), SIGUSR2);
            LOG_INFO(GetLogger()) << "Worker thread signalled; waiting for backtrace capture." << LOG_END;
            worker->backtrace.WaitForCapture();
        }
        if (worker->stored_exception != nullptr) {
            LOG_FATAL(GetLogger()) << "Exception in worker thread " << worker->worker_id << " while executing " << arrow_name << " on event #" << worker->last_event_nr << LOG_END;
        }
    }

    // Now we throw each of these exceptions in order, in case the caller is going to attempt to catch them.
    // In reality all callers are going to print everything they can about the exception and exit.
    for (auto& worker: m_worker_states) {
        if (worker->stored_exception != nullptr) {
            GetApplication()->SetExitCode((int) JApplication::ExitCode::UnhandledException);
            std::rethrow_exception(worker->stored_exception);
        }
        if (worker->is_timed_out) {
            GetApplication()->SetExitCode((int) JApplication::ExitCode::Timeout);
            auto ex = JException("Timeout in worker thread");
            ex.backtrace = worker->backtrace;
            throw ex;
        }
    }
}

void JExecutionEngine::FinishTopology() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Paused);

    LOG_DEBUG(GetLogger()) << "Finishing processing..." << LOG_END;
    for (auto* arrow : m_topology->arrows) {
        arrow->finalize();
    }
    for (auto* pool: m_topology->pools) {
        pool->Finalize();
    }
    m_runstatus = RunStatus::Finished;
    LOG_INFO(GetLogger()) << "Finished processing." << LOG_END;
}

JExecutionEngine::RunStatus JExecutionEngine::GetRunStatus() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_runstatus;
}

JExecutionEngine::Perf JExecutionEngine::GetPerf() {
    std::unique_lock<std::mutex> lock(m_mutex);
    Perf result;
    if (m_runstatus == RunStatus::Paused || m_runstatus == RunStatus::Failed) {
        result.event_count = m_event_count_at_finish - m_event_count_at_start;
        result.uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_time_at_finish - m_time_at_start).count();
    }
    else {
        // Obtain current event count
        size_t current_event_count = 0;
        for (auto& state : m_arrow_states) {
            if (state.is_sink) {
                current_event_count += state.events_processed;
            }
        }
        result.event_count = current_event_count - m_event_count_at_start;
        result.uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - m_time_at_start).count();
    }
    result.runstatus = m_runstatus;
    result.thread_count = m_worker_states.size();
    result.throughput_hz = (result.uptime_ms == 0) ? 0 : (result.event_count * 1000.0) / result.uptime_ms;
    result.event_level = JEventLevel::PhysicsEvent;
    return result;
}

JExecutionEngine::Worker JExecutionEngine::RegisterWorker() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto worker_id = m_worker_states.size();
    auto worker = std::make_unique<WorkerState>();
    worker->worker_id = worker_id;
    worker->is_stop_requested = false;
    worker->cpu_id = m_topology->mapping.get_cpu_id(worker_id);
    worker->location_id = m_topology->mapping.get_loc_id(worker_id);
    worker->thread = nullptr;
    m_worker_states.push_back(std::move(worker));

    bool pin_to_cpu = (m_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
    if (pin_to_cpu) {
        JCpuInfo::PinThreadToCpu(worker->thread, worker->cpu_id);
    }
    return {worker_id, &worker->backtrace};

}


void JExecutionEngine::RunWorker(Worker worker) {

    LOG_DEBUG(GetLogger()) << "Launched worker thread " << worker.worker_id << LOG_END;
    jana2_worker_id = worker.worker_id;
    jana2_worker_backtrace = worker.backtrace;
    try {
        Task task;
        while (true) {
            ExchangeTask(task, worker.worker_id);
            if (task.arrow == nullptr) break; // Exit as soon as ExchangeTask() stops blocking
            task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        }
        LOG_DEBUG(GetLogger()) << "Stopped worker thread " << worker.worker_id << LOG_END;
    }
    catch (...) {
        LOG_ERROR(GetLogger()) << "Exception on worker thread " << worker.worker_id << LOG_END;
        std::unique_lock<std::mutex> lock(m_mutex);
        m_runstatus = RunStatus::Failed;
        m_worker_states.at(worker.worker_id)->stored_exception = std::current_exception();
    }
}


void JExecutionEngine::ExchangeTask(Task& task, size_t worker_id, bool nonblocking) {

    auto checkin_time = std::chrono::steady_clock::now();
    // It's important to start measuring this _before_ acquiring the lock because acquiring the lock
    // may be a big part of the scheduler overhead

    std::unique_lock<std::mutex> lock(m_mutex);
    
    auto& worker = *m_worker_states.at(worker_id);

    if (task.arrow != nullptr) {
        CheckinCompletedTask_Unsafe(task, worker, checkin_time);
    }

    if (worker.is_stop_requested) {
        return;
    }

    FindNextReadyTask_Unsafe(task, worker);

    if (nonblocking) { return; }
    auto idle_time_start = clock_t::now();
    m_total_scheduler_duration += (idle_time_start - checkin_time);

    while (task.arrow == nullptr && !worker.is_stop_requested) {
        m_condvar.wait(lock);
        FindNextReadyTask_Unsafe(task, worker);
    }
    worker.last_checkout_time = clock_t::now();

    if (task.input_event != nullptr) {
        worker.last_event_nr = task.input_event->GetEventNumber();
    }
    else {
        worker.last_event_nr = 0;
    }
    m_total_idle_duration += (worker.last_checkout_time - idle_time_start);

    // Notify one worker, who will notify the next, etc, as long as FindNextReadyTaskUnsafe() succeeds.
    // After FindNextReadyTaskUnsafe fails, all threads block until the next returning worker reactivates the
    // notification chain.
    m_condvar.notify_one();
}


void JExecutionEngine::CheckinCompletedTask_Unsafe(Task& task, WorkerState& worker, clock_t::time_point checkin_time) {

    auto processing_duration = checkin_time - worker.last_checkout_time;

    ArrowState& arrow_state = m_arrow_states.at(worker.last_arrow_id);

    arrow_state.active_tasks -= 1;
    arrow_state.total_processing_duration += processing_duration;

    for (size_t output=0; output<task.output_count; ++output) {
        if (!task.arrow->get_port(task.outputs[output].second).is_input) {
            arrow_state.events_processed++;
        }
    }

    // Put each output in its correct queue or pool
    task.arrow->push(task.outputs, task.output_count, worker.location_id);

    if (task.status == JArrow::FireResult::Finished) {
        // If this is an eventsource self-terminating (the only thing that returns Status::Finished right now) it will
        // have already called DoClose(). I'm tempted to always call DoClose() as part of JExecutionEngine::Finish() instead, however.

        // Mark arrow as finished
        arrow_state.status = ArrowState::Status::Finished;

        // Check if this switches the topology to Draining()
        if (m_runstatus == RunStatus::Running) {
            bool draining = true;
            for (auto& arrow: m_arrow_states) {
                if (arrow.is_source && arrow.status == ArrowState::Status::Running) {
                    draining = false;
                }
            }
            if (draining) {
                m_runstatus = RunStatus::Draining;
            }
        }
    }
    worker.last_arrow_id = -1;
    worker.last_event_nr = 0;

    task.arrow = nullptr;
    task.input_event = nullptr;
    task.output_count = 0;
    task.status = JArrow::FireResult::NotRunYet;
};


void JExecutionEngine::FindNextReadyTask_Unsafe(Task& task, WorkerState& worker) {

    if (m_runstatus == RunStatus::Running || m_runstatus == RunStatus::Draining) {
        // We only pick up a new task if the topology is running or draining.

        // Each call to FindNextReadyTask_Unsafe() starts with a different m_next_arrow_id to ensure balanced arrow assignments
        size_t arrow_count = m_arrow_states.size();
        m_next_arrow_id += 1;
        m_next_arrow_id %= arrow_count;

        for (size_t i=m_next_arrow_id; i<(m_next_arrow_id+arrow_count); ++i) {
            size_t arrow_id = i % arrow_count;

            auto& state = m_arrow_states[arrow_id];
            if (!state.is_parallel && (state.active_tasks != 0)) {
                // We've found a sequential arrow that is already active. Nothing we can do here.
                LOG_TRACE(GetLogger()) << "Scheduler: Arrow with id " << arrow_id << " is unready: Sequential and already active." << LOG_END;
                continue;
            }

            if (state.status != ArrowState::Status::Running) {
                LOG_TRACE(GetLogger()) << "Scheduler: Arrow with id " << arrow_id << " is unready: Arrow is either paused or finished." << LOG_END;
                continue;
            }
            // TODO: Support next_visit_time so that we don't hammer blocked event sources

            // See if we can obtain an input event (this is silly)
            JArrow* arrow = m_topology->arrows[arrow_id];
            // TODO: consider setting state.next_input, retrieving via fire()
            auto port = arrow->get_next_port_index();
            JEvent* event = (port == -1) ? nullptr : arrow->pull(port, worker.location_id);
            if (event != nullptr || port == -1) {
                LOG_TRACE(GetLogger()) << "Scheduler: Found next ready arrow with id " << arrow_id << LOG_END;
                // We've found a task that is ready!
                state.active_tasks += 1;

                task.arrow = arrow;
                task.input_port = port;
                task.input_event = event;
                task.output_count = 0;
                task.status = JArrow::FireResult::NotRunYet;

                worker.last_arrow_id = arrow_id;
                if (event != nullptr) {
                    worker.is_event_warmed_up = event->IsWarmedUp();
                    worker.last_event_nr = event->GetEventNumber();
                }
                else {
                    worker.is_event_warmed_up = true; // Use shorter timeout
                    worker.last_event_nr = 0;
                }
                return;
            }
            else {
                LOG_TRACE(GetLogger()) << "Scheduler: Arrow with id " << arrow_id << " is unready: Input event is needed but not on queue yet." << LOG_END;
            }
        }
    }

    // Because we reached this point, we know that there aren't any tasks ready,
    // so we check whether more are potentially coming. If not, we can pause the topology.
    // Note that our worker threads will still wait at ExchangeTask() until they get
    // shut down separately during Scale().
    
    if (m_runstatus == RunStatus::Running || m_runstatus == RunStatus::Pausing || m_runstatus == RunStatus::Draining) {
        // We want to avoid scenarios such as where the topology already Finished but then gets reset to Paused
        // This also leaves a cleaner narrative in the logs. 

        bool any_active_source_found = false;
        bool any_active_task_found = false;
        
        LOG_DEBUG(GetLogger()) << "Scheduler: No tasks ready" << LOG_END;

        for (size_t arrow_id = 0; arrow_id < m_arrow_states.size(); ++arrow_id) {
            auto& state = m_arrow_states[arrow_id];
            any_active_source_found |= (state.status == ArrowState::Status::Running && state.is_source);
            any_active_task_found |= (state.active_tasks != 0);
            // A source might have been deactivated by RequestPause, Ctrl-C, etc, and might be inactive even though it still has active tasks
        }

        if (!any_active_source_found && !any_active_task_found) {
            // Pause the topology
            m_time_at_finish = clock_t::now();
            m_event_count_at_finish = 0;
            for (auto& arrow_state : m_arrow_states) {
                if (arrow_state.is_sink) {
                    m_event_count_at_finish += arrow_state.events_processed;
                }
            }
            LOG_DEBUG(GetLogger()) << "Scheduler: Processing paused" << LOG_END;
            m_runstatus = RunStatus::Paused;
            // I think this is the ONLY site where the topology gets paused. Verify this?
        }
    }

    worker.last_arrow_id = -1;

    task.arrow = nullptr;
    task.input_port = -1;
    task.input_event = nullptr;
    task.output_count = 0;
    task.status = JArrow::FireResult::NotRunYet;
}


void JExecutionEngine::PrintFinalReport() {

    std::unique_lock<std::mutex> lock(m_mutex);
    auto event_count = m_event_count_at_finish - m_event_count_at_start;
    auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_time_at_finish - m_time_at_start).count();
    auto thread_count = m_worker_states.size();
    auto throughput_hz = (event_count * 1000.0) / uptime_ms;

    LOG_INFO(GetLogger()) << "Detailed report:" << LOG_END;
    LOG_INFO(GetLogger()) << LOG_END;
    LOG_INFO(GetLogger()) << "  Avg throughput [Hz]:         " << std::setprecision(3) << throughput_hz << LOG_END;
    LOG_INFO(GetLogger()) << "  Completed events [count]:    " << event_count << LOG_END;
    LOG_INFO(GetLogger()) << "  Total uptime [s]:            " << std::setprecision(4) << uptime_ms/1000.0 << LOG_END;
    LOG_INFO(GetLogger()) << "  Thread team size [count]:    " << thread_count << LOG_END;
    LOG_INFO(GetLogger()) << LOG_END;
    LOG_INFO(GetLogger()) << "  Arrow-level metrics:" << LOG_END;
    LOG_INFO(GetLogger()) << LOG_END;

    size_t total_useful_ms = 0;

    for (size_t arrow_id=0; arrow_id < m_arrow_states.size(); ++arrow_id) {
        auto* arrow = m_topology->arrows[arrow_id];
        auto& arrow_state = m_arrow_states[arrow_id];
        auto useful_ms = std::chrono::duration_cast<std::chrono::milliseconds>(arrow_state.total_processing_duration).count();
        total_useful_ms += useful_ms;
        auto avg_latency = useful_ms*1.0/arrow_state.events_processed;
        auto throughput_bottleneck = 1000.0 / avg_latency;
        if (arrow->is_parallel()) {
            throughput_bottleneck *= thread_count;
        }

        LOG_INFO(GetLogger()) << "  - Arrow name:                 " << arrow->get_name() << LOG_END;
        LOG_INFO(GetLogger()) << "    Parallel:                   " << arrow->is_parallel() << LOG_END;
        LOG_INFO(GetLogger()) << "    Events completed:           " << arrow_state.events_processed << LOG_END;
        LOG_INFO(GetLogger()) << "    Avg latency [ms/event]:     " << avg_latency << LOG_END;
        LOG_INFO(GetLogger()) << "    Throughput bottleneck [Hz]: " << throughput_bottleneck << LOG_END;
        LOG_INFO(GetLogger()) << LOG_END;
    }

    auto total_scheduler_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_total_scheduler_duration).count();
    auto total_idle_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_total_idle_duration).count();

    LOG_INFO(GetLogger()) << "  Total useful time [s]:     " << std::setprecision(6) << total_useful_ms/1000.0 << LOG_END;
    LOG_INFO(GetLogger()) << "  Total scheduler time [s]:  " << std::setprecision(6) << total_scheduler_ms/1000.0 << LOG_END;
    LOG_INFO(GetLogger()) << "  Total idle time [s]:       " << std::setprecision(6) << total_idle_ms/1000.0 << LOG_END;

    LOG_INFO(GetLogger()) << LOG_END;

    LOG_INFO(GetLogger()) << "Final report: " << event_count << " events processed at "
                          << JTypeInfo::to_string_with_si_prefix(throughput_hz) << "Hz" << LOG_END;

}

void JExecutionEngine::SetTickerEnabled(bool show_ticker) {
    m_show_ticker = show_ticker;
}

bool JExecutionEngine::IsTickerEnabled() const {
    return m_show_ticker;
}

void JExecutionEngine::SetTimeoutEnabled(bool timeout_enabled) {
    m_enable_timeout = timeout_enabled;
}

bool JExecutionEngine::IsTimeoutEnabled() const {
    return m_enable_timeout;
}

JArrow::FireResult JExecutionEngine::Fire(size_t arrow_id, size_t location_id) {

    std::unique_lock<std::mutex> lock(m_mutex);
    if (arrow_id >= m_topology->arrows.size()) {
        LOG_WARN(GetLogger()) << "Firing unsuccessful: No arrow exists with id=" << arrow_id << LOG_END;
        return JArrow::FireResult::NotRunYet;
    }
    JArrow* arrow = m_topology->arrows[arrow_id];
    LOG_WARN(GetLogger()) << "Attempting to fire arrow with name=" << arrow->get_name() 
                          << ", index=" << arrow_id << ", location=" << location_id << LOG_END;

    ArrowState& arrow_state = m_arrow_states[arrow_id];
    if (arrow_state.status == ArrowState::Status::Finished) {
        LOG_WARN(GetLogger()) << "Firing unsuccessful: Arrow status is Finished." << arrow_id << LOG_END;
        return JArrow::FireResult::Finished;
    }
    if (!arrow_state.is_parallel && arrow_state.active_tasks != 0) {
        LOG_WARN(GetLogger()) << "Firing unsuccessful: Arrow is sequential and already has an active task." << arrow_id << LOG_END;
        return JArrow::FireResult::NotRunYet;
    }
    arrow_state.active_tasks += 1;

    auto port = arrow->get_next_port_index();
    JEvent* event = nullptr;
    if (port != -1) {
        event = arrow->pull(port, location_id);
        if (event == nullptr) {
            LOG_WARN(GetLogger()) << "Firing unsuccessful: Arrow needs an input event from port " << port << ", but the queue or pool is empty." << LOG_END;
            arrow_state.active_tasks -= 1;
            return JArrow::FireResult::NotRunYet;
        }
        else {
            LOG_WARN(GetLogger()) << "Input event #" << event->GetEventNumber() << " from port " << port << LOG_END;
        }
    }
    else {
        LOG_WARN(GetLogger()) << "No input events" << LOG_END;
    }
    lock.unlock();

    size_t output_count;
    JArrow::OutputData outputs;
    JArrow::FireResult result = JArrow::FireResult::NotRunYet;

    LOG_WARN(GetLogger()) << "Firing arrow" << LOG_END;
    arrow->fire(event, outputs, output_count, result);
    LOG_WARN(GetLogger()) << "Fired arrow with result " << to_string(result) << LOG_END;
    if (output_count == 0) {
        LOG_WARN(GetLogger()) << "No output events" << LOG_END;
    }
    else {
        for (size_t i=0; i<output_count; ++i) {
            LOG_WARN(GetLogger()) << "Output event #" << outputs.at(i).first->GetEventNumber() << " on port " << outputs.at(i).second << LOG_END;
        }
    }

    lock.lock();
    arrow->push(outputs, output_count, location_id);
    arrow_state.active_tasks -= 1;
    lock.unlock();
    return result;
}


void JExecutionEngine::HandleSIGINT() {
    InterruptStatus status = m_interrupt_status;
    std::cout << std::endl;
    switch (status) {
        case InterruptStatus::NoInterruptsSupervised: m_interrupt_status = InterruptStatus::InspectRequested; break;
        case InterruptStatus::InspectRequested: m_interrupt_status = InterruptStatus::PauseAndQuit; break;
        case InterruptStatus::NoInterruptsUnsupervised:
        case InterruptStatus::PauseAndQuit:
        case InterruptStatus::InspectInProgress: 
            _exit(-2);
    }
}

void JExecutionEngine::HandleSIGUSR1() {
    m_send_worker_report_requested = true;
}

void JExecutionEngine::HandleSIGUSR2() {
    if (jana2_worker_backtrace != nullptr) {
        jana2_worker_backtrace->Capture(3);
    }
}

void JExecutionEngine::HandleSIGTSTP() {
    std::cout << std::endl;
    m_print_worker_report_requested = true;
}

void JExecutionEngine::PrintWorkerReport(bool send_to_pipe) {

    std::unique_lock<std::mutex> lock(m_mutex);
    LOG_INFO(GetLogger()) << "Generating worker report. It may take some time to retrieve each symbol's debug information." << LOG_END;
    for (auto& worker: m_worker_states) {
        worker->backtrace.Reset();
        pthread_kill(worker->thread->native_handle(), SIGUSR2);
    }
    for (auto& worker: m_worker_states) {
        worker->backtrace.WaitForCapture();
    }
    std::ostringstream oss;
    oss << "Worker report" << std::endl;
    for (auto& worker: m_worker_states) {
        oss << "------------------------------" << std::endl 
            << "  Worker:        " << worker->worker_id << std::endl
            << "  Current arrow: " << worker->last_arrow_id << std::endl
            << "  Current event: " << worker->last_event_nr << std::endl
            << "  Backtrace:" << std::endl << std::endl
            << worker->backtrace.ToString();
    }
    auto s = oss.str();
    LOG_WARN(GetLogger()) << s << LOG_END;

    if (send_to_pipe) {

        int fd = open(m_path_to_named_pipe.c_str(), O_WRONLY);
        if (fd >= 0) {
            write(fd, s.c_str(), s.length()+1);
            close(fd);
        }
        else {
            LOG_ERROR(GetLogger()) << "Unable to open named pipe '" << m_path_to_named_pipe << "' for writing. \n"
            << "  You can use a different named pipe for status info by setting the parameter `jana:status_fname`.\n"
            << "  The status report will still show up in the log." << LOG_END;
        }
    }
}


std::string ToString(JExecutionEngine::RunStatus runstatus) {
    switch(runstatus) {
        case JExecutionEngine::RunStatus::Running: return "Running";
        case JExecutionEngine::RunStatus::Paused: return "Paused";
        case JExecutionEngine::RunStatus::Failed: return "Failed";
        case JExecutionEngine::RunStatus::Pausing: return "Pausing";
        case JExecutionEngine::RunStatus::Draining: return "Draining";
        case JExecutionEngine::RunStatus::Finished: return "Finished";
        default: return "CorruptedRunStatus";
    }
}

