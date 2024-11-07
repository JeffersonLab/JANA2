
#include "JExecutionEngine.h"
#include "JANA/Topology/JArrowMetrics.h"

#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <mutex>


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


    // Not sure how I feel about putting this here yet, but I think it will at least work in both cases it needs to.
    // The reason this works is because JTopologyBuilder::create_topology() has already been called before 
    // JApplication::ProvideService<JExecutionEngine>().
    for (JArrow* arrow : m_topology->arrows) {
        m_scheduler_state.emplace_back();
        auto& state = m_scheduler_state.back();
        state.is_source = arrow->is_source();
        state.is_sink = arrow->is_sink();
        state.is_parallel = arrow->is_parallel();
        state.next_input = arrow->get_next_port_index();
    }
}


void JExecutionEngine::Run() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Paused);

    // Set start time and event count
    m_time_at_start = clock_t::now();
    m_event_count_at_start = m_event_count_at_finish;

    m_runstatus = RunStatus::Running;
    // Put some initial tasks in the active task queue?

    lock.unlock();
    m_condvar.notify_one();
}

void JExecutionEngine::Scale(size_t nthreads) {
    // We both create and destroy the pool of workers here. They all sleep until they 
    // receive work from the scheduler, which won't happen until the runstatus <- {Running, 
    // Pausing, Draining} and there is a task ready to execute. This way worker creation/destruction
    // is decoupled from topology execution.

    // If we scale to zero, no workers will run. This is useful for testing, and also for using
    // an external thread team, should the need arise.

    std::unique_lock<std::mutex> lock(m_mutex);

    assert(m_runstatus == RunStatus::Paused || m_runstatus == RunStatus::Finished);

    auto prev_nthreads = m_workers.size();

    if (prev_nthreads < nthreads) {
        // We are launching additional worker threads
        LOG_DEBUG(GetLogger()) << "Scaling up to " << nthreads << " worker threads" << LOG_END;
        for (size_t worker_id=prev_nthreads; worker_id < nthreads; ++worker_id) {
            auto worker = std::make_unique<Worker>();
            worker->worker_id = worker_id;
            worker->is_stop_requested = false;
            worker->cpu_id = m_topology->mapping.get_cpu_id(worker_id);
            worker->location_id = m_topology->mapping.get_loc_id(worker_id);
            worker->thread = new std::thread(&JExecutionEngine::RunWorker, this, std::ref(*worker));
            m_workers.push_back(std::move(worker));

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
            m_workers[worker_id]->is_stop_requested = true;
        }
        lock.unlock();

        m_condvar.notify_all(); // Wake up all threads so that they can exit the condvar wait loop

        // We join all (eligible) threads _outside_ of the mutex
        for (int worker_id=prev_nthreads-1; worker_id >= (int) nthreads; --worker_id) {
            if (m_workers[worker_id]->thread != nullptr) {
                if (m_workers[worker_id]->is_timed_out) {
                    // Thread has timed out. Rather than non-cooperatively killing it,
                    // we relinquish ownership of it but remember that it was ours once and
                    // is still out there, somewhere, biding its time
                    m_workers[worker_id]->thread->detach();
                    LOG_DEBUG(GetLogger()) << "Detached worker " << worker_id << LOG_END;
                }
                else {
                    LOG_DEBUG(GetLogger()) << "Joining worker " << worker_id << LOG_END;
                    m_workers[worker_id]->thread->join();
                    LOG_DEBUG(GetLogger()) << "Joined worker " << worker_id << LOG_END;
                }
            }
            else {
                LOG_DEBUG(GetLogger()) << "Skipping worker " << worker_id << LOG_END;
            }
        }

        lock.lock();
        // We retake the mutex so we can safely modify m_workers
        for (int worker_id=prev_nthreads-1; worker_id >= (int)nthreads; --worker_id) {
            if (m_workers.back()->thread != nullptr) {
                delete m_workers.back()->thread;
            }
            m_workers.pop_back();
        }
    }
}

void JExecutionEngine::RequestPause() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Running);
    m_runstatus = RunStatus::Pausing;
    for (auto& arrow: m_scheduler_state) {
        arrow.is_active = false;
    }
    LOG_WARN(GetLogger()) << "Requested pause" << LOG_END;
    lock.unlock();
    m_condvar.notify_all();
}

void JExecutionEngine::RequestDrain() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Running);
    m_runstatus = RunStatus::Draining;
    for (auto& arrow: m_scheduler_state) {
        if (arrow.is_source) {
            arrow.is_active = false;
        }
    }
    LOG_WARN(GetLogger()) << "Requested drain" << LOG_END;
    lock.unlock();
    m_condvar.notify_all();
}

void JExecutionEngine::Wait(bool finish) {

    size_t last_event_count = 0;
    clock_t::time_point last_measurement_time = clock_t::now();

    Perf perf;
    while (true) {

        if (m_enable_timeout) {
            CheckTimeout();
        }
        perf = GetPerf();
        if (perf.runstatus == RunStatus::Paused || perf.runstatus == RunStatus::Finished) {
            break;
        }

        if (m_show_ticker) {
            auto last_measurement_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - last_measurement_time).count();
            float latest_throughput_hz = (last_measurement_duration_ms == 0) ? 0 : (perf.event_count - last_event_count) * 1000.0 / last_measurement_duration_ms;
            last_measurement_time = clock_t::now();
            last_event_count = perf.event_count;

            // Print rates
            LOG_WARN(m_logger) << "Status: " << perf.event_count << " events processed at "
                            << JTypeInfo::to_string_with_si_prefix(latest_throughput_hz) << "Hz ("
                            << JTypeInfo::to_string_with_si_prefix(perf.throughput_hz) << "Hz avg)" << LOG_END;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_ticker_ms));
    }
    LOG_INFO(GetLogger()) << "Processing paused." << LOG_END;

    if (perf.runstatus == RunStatus::Failed) {
        HandleFailures();
    }

    if (finish) {
        Finish();
    }
    PrintFinalReport();
}

bool JExecutionEngine::CheckTimeout() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = clock_t::now();
    bool timeout_detected = false;
    for (auto& worker: m_workers) {
        auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(now - worker->last_checkout_time).count();
        if (duration_s > m_timeout_s) {
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
    bool timeout_found = false;
    for (auto& worker: m_workers) {
        if (worker->is_timed_out) {
            LOG_FATAL(GetLogger()) << "Timeout in worker thread " << worker->worker_id << LOG_END;
            timeout_found = true;
        }
        if (worker->stored_exception != nullptr) {
            LOG_FATAL(GetLogger()) << "Exception in worker thread " << worker->worker_id << LOG_END;
        }
    }

    // Now we throw each of these exceptions in order, in case the caller is going to attempt to catch them.
    // In reality all callers are going to print everything they can about the exception and exit.
    for (auto& worker: m_workers) {
        if (worker->stored_exception != nullptr) {
            throw worker->stored_exception;
        }
    }
    if (timeout_found) {
        throw JException("Worker timeout!");
    }
}

void JExecutionEngine::Finish() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Paused);

    LOG_DEBUG(GetLogger()) << "Finishing processing..." << LOG_END;
    for (auto* arrow : m_topology->arrows) {
        arrow->finalize();
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
        for (auto& state : m_scheduler_state) {
            if (state.is_sink) {
                current_event_count += state.events_processed;
            }
        }
        result.event_count = current_event_count - m_event_count_at_start;
        result.uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - m_time_at_start).count();
    }
    result.runstatus = m_runstatus;
    result.thread_count = m_workers.size();
    result.throughput_hz = (result.uptime_ms == 0) ? 0 : (result.event_count * 1000.0) / result.uptime_ms;
    result.event_level = JEventLevel::PhysicsEvent;
    return result;
}

int JExecutionEngine::RegisterExternalWorker() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto worker_id = m_workers.size();
    auto worker = std::make_unique<Worker>();
    worker->worker_id = worker_id;
    worker->is_stop_requested = false;
    worker->cpu_id = m_topology->mapping.get_cpu_id(worker_id);
    worker->location_id = m_topology->mapping.get_loc_id(worker_id);
    worker->thread = nullptr;
    m_workers.push_back(std::move(worker));

    bool pin_to_cpu = (m_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
    if (pin_to_cpu) {
        JCpuInfo::PinThreadToCpu(worker->thread, worker->cpu_id);
    }
    return worker_id;

}


void JExecutionEngine::RunWorker(Worker& worker) {
    Task task;
    task.worker_id = worker.worker_id;

    LOG_DEBUG(GetLogger()) << "Launched worker thread " << worker.worker_id << ". cpu=" << worker.cpu_id << ", location=" << worker.location_id << LOG_END;
    try {
        while (true) {
            ExchangeTask(task);
            if (task.arrow == nullptr) break; // Exit as soon as ExchangeTask() stops blocking
            task.arrow->fire(task.input_event, task.outputs, task.output_count, task.status);
        }
        LOG_DEBUG(GetLogger()) << "Stopped worker thread " << worker.worker_id << LOG_END;
    }
    catch (...) {
        LOG_ERROR(GetLogger()) << "Exception on worker thread " << worker.worker_id << LOG_END;
        std::unique_lock<std::mutex> lock(m_mutex);
        m_runstatus = RunStatus::Failed;
        worker.stored_exception = std::current_exception();
    }
}


void JExecutionEngine::ExchangeTask(Task& task, bool nonblocking) {

    auto checkin_time = std::chrono::steady_clock::now();
    // It's important to start measuring this _before_ acquiring the lock because acquiring the lock
    // may be a big part of the scheduler overhead

    std::unique_lock<std::mutex> lock(m_mutex);

    if (task.arrow != nullptr) {
        CheckinCompletedTask_Unsafe(task, checkin_time);
    }

    if (task.worker_id == -1 || task.worker_id >= (int) m_workers.size()) {
        // This happens if we are an external worker and someone called Scale() to deactivate us
        return;
    }
    auto& worker = *m_workers[task.worker_id];
    if (worker.is_stop_requested) {
        return;
    }

    FindNextReadyTask_Unsafe(task);

    if (nonblocking) { return; }
    auto idle_time_start = clock_t::now();
    m_total_scheduler_duration += (idle_time_start - checkin_time);

    while (task.arrow == nullptr && !worker.is_stop_requested) {
        m_condvar.wait(lock);
        FindNextReadyTask_Unsafe(task);
    }
    worker.last_checkout_time = clock_t::now();
    m_total_idle_duration += (worker.last_checkout_time - idle_time_start);

    lock.unlock();
    // Notify one worker, who will notify the next, etc, as long as FindNextReadyTaskUnsafe() succeeds.
    // After FindNextReadyTaskUnsafe fails, all threads block until the next returning worker reactivates the
    // notification chain.
    m_condvar.notify_one();
}


void JExecutionEngine::CheckinCompletedTask_Unsafe(Task& task, clock_t::time_point checkin_time) {

    auto& worker = *m_workers.at(task.worker_id);
    auto processing_duration = checkin_time - worker.last_checkout_time;

    SchedulerState& arrow_state = m_scheduler_state.at(task.arrow_id);

    arrow_state.active_tasks -= 1;
    arrow_state.total_processing_duration += processing_duration;

    for (size_t output=0; output<task.output_count; ++output) {
        if (!task.arrow->get_port(task.outputs[output].second).is_input) {
            arrow_state.events_processed++;
        }

        // Put each output in its correct queue or pool
        task.arrow->push(task.outputs, task.output_count, worker.location_id);
    }

    if (task.status == JArrowMetrics::Status::Finished) {
        // If this is an eventsource self-terminating (the only thing that returns Status::Finished right now) it will
        // have already called DoClose(). I'm tempted to always call DoClose() as part of JExecutionEngine::Finish() instead, however.

        // Mark arrow as finished
        arrow_state.is_active = false;

        // Check if this switches the topology to Draining()
        if (m_runstatus == RunStatus::Running) {
            bool draining = true;
            for (auto& arrow: m_scheduler_state) {
                if (arrow.is_source && arrow.is_active) {
                    draining = false;
                }
            }
            if (draining) {
                m_runstatus = RunStatus::Draining;
            }
        }
    }
    task.arrow = nullptr;
    task.arrow_id = -1;
    task.input_event = nullptr;
    task.output_count = 0;
    task.status = JArrowMetrics::Status::NotRunYet;
};


void JExecutionEngine::FindNextReadyTask_Unsafe(Task& task) {

    auto& worker = *m_workers.at(task.worker_id);

    if (m_runstatus == RunStatus::Running || m_runstatus == RunStatus::Draining) {
        // We only pick up a new task if the topology is running or draining.

        for (size_t arrow_id=0; arrow_id<m_scheduler_state.size(); ++arrow_id) {

            auto& state = m_scheduler_state[arrow_id];
            if (!state.is_parallel && (state.active_tasks != 0)) {
                // We've found a sequential arrow that is already running. Nothing we can do here.
                continue;
            }

            if (!state.is_active) {
                continue;
            }
            // TODO: Support next_visit_time so that we don't hammer blocked event sources

            // See if we can obtain an input event (this is silly)
            JArrow* arrow = m_topology->arrows[arrow_id];
            // TODO: consider setting state.next_input, retrieving via fire()
            auto port = arrow->get_next_port_index();
            JEvent* event = arrow->pull(port, worker.location_id);
            if (event != nullptr) {
                // We've found a task that is ready!
                state.active_tasks += 1;

                task.arrow_id = arrow_id;
                task.arrow = arrow;
                task.input_port = port;
                task.input_event = event;
                task.output_count = 0;
                task.status = JArrowMetrics::Status::NotRunYet;
                return;
            }
        }
    }

    // Because we reached this point, we know that there aren't any tasks ready,
    // so we check whether more are potentially coming. If not, we can pause the topology.
    // Note that our worker threads will still wait at ExchangeTask() until they get
    // shut down separately during Scale().

    bool any_active_source_found = false;
    bool any_active_task_found = false;
    
    LOG_DEBUG(GetLogger()) << "Scheduler: No tasks ready" << LOG_END;

    for (size_t arrow_id = 0; arrow_id < m_scheduler_state.size(); ++arrow_id) {
        auto& state = m_scheduler_state[arrow_id];
        auto* arrow = m_topology->arrows[arrow_id];
        LOG_TRACE(GetLogger()) << "Scheduler: arrow=" << arrow->get_name() << ", is_source=" << state.is_source << ", active_tasks=" << state.active_tasks << ", is_parallel=" << state.is_parallel << LOG_END;
        any_active_source_found |= (state.is_active && state.is_source);
        any_active_task_found |= (state.active_tasks != 0);
        // A source might have been deactivated by RequestPause, Ctrl-C, etc, and might be inactive even though it still has active tasksa
    }

    if (!any_active_source_found && !any_active_task_found) {
        // Pause the topology
        m_time_at_finish = clock_t::now();
        m_event_count_at_finish = 0;
        for (auto& state : m_scheduler_state) {
            if (state.is_sink) {
                m_event_count_at_finish += state.events_processed;
            }
        }
        LOG_DEBUG(GetLogger()) << "Processing paused" << LOG_END;
        m_runstatus = RunStatus::Paused;
        // I think this is the ONLY site where the topology gets paused. Verify this?
    }

    task.arrow_id = -1;
    task.arrow = nullptr;
    task.input_port = -1;
    task.input_event = nullptr;
    task.output_count = 0;
    task.status = JArrowMetrics::Status::NotRunYet;
}


void JExecutionEngine::PrintFinalReport() {

    std::unique_lock<std::mutex> lock(m_mutex);
    auto event_count = m_event_count_at_finish - m_event_count_at_start;
    auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_time_at_finish - m_time_at_start).count();
    auto thread_count = m_workers.size();
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

    for (size_t arrow_id=0; arrow_id < m_scheduler_state.size(); ++arrow_id) {
        auto* arrow = m_topology->arrows[arrow_id];
        auto& arrow_state = m_scheduler_state[arrow_id];
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
    auto total_idle_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_total_scheduler_duration).count();

    LOG_INFO(GetLogger()) << "  Total useful time [s]:     " << std::setprecision(4) << total_useful_ms/1000.0 << LOG_END;
    LOG_INFO(GetLogger()) << "  Total scheduler time [s]:  " << std::setprecision(4) << total_scheduler_ms/1000.0 << LOG_END;
    LOG_INFO(GetLogger()) << "  Total idle time [s]:       " << std::setprecision(4) << total_idle_ms/1000.0 << LOG_END;

    LOG_INFO(GetLogger()) << LOG_END;

    LOG_WARN(GetLogger()) << "Final report: " << event_count << " events processed at "
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


