
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
}

void JExecutionEngine::RequestDrain() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Running);
    m_runstatus = RunStatus::Draining;
}

void JExecutionEngine::Wait(bool finish) {

    size_t last_event_count = 0;
    clock_t::time_point last_measurement_time = clock_t::now();

    while (true) {
        auto perf = GetPerf();
        if (perf.runstatus == RunStatus::Paused || perf.runstatus == RunStatus::Finished) {
            break;
        }

        if (m_show_ticker) {
            auto last_measurement_duration = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - last_measurement_time);
            float latest_throughput_hz = (perf.event_count - last_event_count) * 1000.0 / last_measurement_duration.count();
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

    if (finish) {
        Finish();
        LOG_INFO(GetLogger()) << "Processing finished." << LOG_END;
    }

    auto perf = GetPerf();
    LOG_INFO(GetLogger()) << "Detailed report:" << LOG_END;
    LOG_INFO(GetLogger()) << LOG_END;
    LOG_INFO(GetLogger()) << "  Thread team size [count]:    " << perf.thread_count << LOG_END;
    LOG_INFO(GetLogger()) << "  Total uptime [s]:            " << std::setprecision(4) << perf.uptime_ms/1000 << LOG_END;
    LOG_INFO(GetLogger()) << "  Completed events [count]:    " << perf.event_count << LOG_END;
    LOG_INFO(GetLogger()) << "  Avg throughput [Hz]:         " << std::setprecision(3) << perf.throughput_hz << LOG_END;
    LOG_INFO(GetLogger()) << LOG_END;
    //LOG_INFO(GetLogger()) << "  Sequential bottleneck [Hz]:  " << std::setprecision(3) << metrics->avg_seq_bottleneck_hz << LOG_END;
    //LOG_INFO(GetLogger()) << "  Parallel bottleneck [Hz]:    " << std::setprecision(3) << metrics->avg_par_bottleneck_hz << LOG_END;
    //LOG_INFO(GetLogger()) << "  Efficiency [0..1]:           " << std::setprecision(3) << metrics->avg_efficiency_frac << LOG_END;
    
    LOG_WARN(GetLogger()) << "Final report: " << perf.event_count << " events processed at "
                       << JTypeInfo::to_string_with_si_prefix(perf.throughput_hz) << "Hz" << LOG_END;
}

void JExecutionEngine::Finish() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Paused);
    m_runstatus = RunStatus::Finished;
    // Close everything _outside_ the mutex?
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
    result.throughput_hz = (result.event_count * 1000.0) / result.uptime_ms;
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
            if (task.arrow == nullptr) break;

            worker.last_checkin_time = clock_t::now();
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

    std::unique_lock<std::mutex> lock(m_mutex);

    if (task.arrow != nullptr) {
        IngestCompletedTask_Unsafe(task);
    }

    if (task.worker_id == -1 || task.worker_id >= (int) m_workers.size()) {
        // This happens if we are an external worker and someone called Scale() to deactivate us
        return;
    }
    auto& worker = m_workers[task.worker_id];
    if (worker->is_stop_requested) {
        return;
    }

    FindNextReadyTask_Unsafe(task);

    if (nonblocking) { return; }

    while (task.arrow == nullptr && !worker->is_stop_requested) {
        m_condvar.wait(lock);
        FindNextReadyTask_Unsafe(task);
    }

    lock.unlock();
    // Notify one worker, who will notify the next, etc, as long as FindNextReadyTaskUnsafe() succeeds.
    // After FindNextReadyTaskUnsafe fails, all threads block until the next returning worker reactivates the
    // notification chain.
    m_condvar.notify_one();
}


void JExecutionEngine::IngestCompletedTask_Unsafe(Task& task) {

    auto ingest_time = std::chrono::steady_clock::now();
    auto& worker = *m_workers.at(task.worker_id);
    auto processing_duration = ingest_time - worker.last_checkin_time;
    // TODO: Warn about slow events that didn't timeout but came close

    SchedulerState& arrow_state = m_scheduler_state.at(task.arrow_id);

    arrow_state.active_tasks -= 1;
    arrow_state.total_processing_duration += processing_duration;

    for (size_t output=0; output<task.output_count; ++output) {

        // Update arrow's processed count. We _could_ also track the arrow latency (and the other arrow metrics) this way, but
        // we would need worker-level information, specifically the start time.
        if (!task.arrow->get_port(task.outputs[output].second).is_input) {
            arrow_state.events_processed++;
        }

        // Put each output in its correct queue or pool
        // We also need (worker-level) location ID
        task.arrow->push(task.outputs, task.output_count, 0);
    }

    if (task.status == JArrowMetrics::Status::Finished) {
        // If this is an eventsource self-terminating (the only thing that returns Status::Finished right now) it will
        // have already called DoClose(). I'm tempted to always call DoClose() as part of JExecutionEngine::Finish() instead, however.

        // Mark arrow as finished
        arrow_state.is_active = false;
    }
    task.arrow = nullptr;
    task.arrow_id = -1;
    task.input_event = nullptr;
    task.output_count = 0;
    task.status = JArrowMetrics::Status::NotRunYet;
};


void JExecutionEngine::FindNextReadyTask_Unsafe(Task& task) {

    // Check to see if the topology itself has shut down
    if (m_runstatus != RunStatus::Running && m_runstatus != RunStatus::Draining) {

        // Because the topology is not running, there's nothing to do
        // This doesn't mean we should terminate, though, because we create our thread team _before_ we call Run().
        // Worker termination is handled through Scale() and needs a per-worker flag. We will have to add this later.

        task.arrow = nullptr;
        task.input_event = nullptr;
        task.input_port = -1;
        return;
    }

    // Because we reached this point, we know that the topology is running, 
    // so we look for work we can do
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
        JEvent* event = arrow->pull(port, 0); // TODO: Need worker location_id
        if (event != nullptr) {
            // We've found a task that is ready!
            // Start timer // TODO: This requires worker
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
        if (state.is_source && !state.is_active) {
            // Sanity check: no active tasks for inactive sources
            assert(state.active_tasks == 0);
        }
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
