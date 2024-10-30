
#include "JExecutionEngine.h"
#include "JANA/Topology/JArrowMetrics.h"
#include "JANA/Utils/JEventLevel.h"
#include <chrono>
#include <cstddef>


void JExecutionEngine::Init() {
    auto params = GetApplication()->GetJParameterManager();

    params->SetDefaultParameter("jana:timeout", m_timeout_s, 
        "Max time (in seconds) JANA will wait for a thread to update its heartbeat before hard-exiting. 0 to disable timeout completely.");

    params->SetDefaultParameter("jana:warmup_timeout", m_warmup_timeout_s, 
        "Max time (in seconds) JANA will wait for 'initial' events to complete before hard-exiting.");

    params->SetDefaultParameter("jana:poll_ms", m_poll_ms, 
        "Max time (in seconds) JANA will wait for 'initial' events to complete before hard-exiting.");

    bool m_pin_to_cpu = (m_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
}


void JExecutionEngine::Run() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Paused);

    // Set start time and event count
    // m_event_count_at_start = m_topology->GetProcessedEventCount();

    m_runstatus = RunStatus::Running;
    // Put some initial tasks in the active task queue?
}

void JExecutionEngine::Scale(size_t nthreads) {
    // We create the pool of workers here. They all sleep until they receive work from the scheduler,
    // which won't happen until the runstatus <- {Running, Pausing, Draining} and there is
    // a task ready to execute.

    // Eventually we might want to 

    // If we scale to zero, no workers will run. This is useful for testing, and also for using
    // an external thread team, should the need arise.

    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Paused);
    m_nthreads = nthreads;
    for (size_t worker_id = 0; worker_id < m_nthreads; ++worker_id) {
        Worker worker;
        worker.worker_id = worker_id;
        worker.cpu_id = m_topology->mapping.get_cpu_id(worker_id);
        worker.location_id = m_topology->mapping.get_loc_id(worker_id);
        // Create thread, possibly pin it
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
    // RunSupervisor until runstatus is either Paused or Failed
    // Who sets runstatus to paused or failed?
    // - Supervisor presumably sets failed
    // - ExchangeTask presumably sets paused

    for (Worker& worker: m_workers) {
        // If worker is excepted or timed out, detach instead
        worker.thread->join();
    }
    m_runstatus = RunStatus::Paused;
    if (finish) {
        Finish();
    }
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
        result.uptime = m_time_at_finish - m_time_at_start;
    }
    else {
        // Obtain current event count
        size_t current_event_count = 2;
        result.event_count = current_event_count - m_event_count_at_start;
        result.uptime = clock_t::now() - m_time_at_start;

    }
    result.thread_count = m_nthreads;
    result.throughput_hz = (result.event_count * 1000.0) / result.uptime.count();
    result.event_level = JEventLevel::PhysicsEvent;
    return result;
}

void JExecutionEngine::RunSupervisor() {
    // Test for timeout
    // Test for stored exceptions
    // Print runstatus ticker
    auto perf = GetPerf();
    std::cout << perf.event_count << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void JExecutionEngine::RunWorker(Worker& worker) {
    Task task;
    try {
        do {
            ExchangeTask(task);
            worker.last_checkin_time = clock_t::now();
            // Do the work
        } while (task.arrow != nullptr);
    }
    catch (JException& e) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_runstatus = RunStatus::Failed;
        worker.stored_exception = e;
    }
}


void JExecutionEngine::ExchangeTask(Task& task) {

    std::unique_lock<std::mutex> lock(m_mutex);

    IngestCompletedTask_Unsafe(task);

    bool found_task = false;
    bool found_termination = false;
    FindNextReadyTask_Unsafe(task, found_task, found_termination);

    while (!found_task && !found_termination) {
        m_condvar.wait(lock);
        FindNextReadyTask_Unsafe(task, found_task, found_termination);
    }

    lock.unlock();
    // Notify one worker, who will notify the next, etc, as long as FindNextReadyTaskUnsafe() succeeds.
    // After FindNextReadyTaskUnsafe fails, all threads block until the next returning worker reactivates the
    // notification chain.
    m_condvar.notify_one();
}


void JExecutionEngine::IngestCompletedTask_Unsafe(Task& task) {
    // IngestCompletedTask_Unsafe() handles all bookkeeping

    auto ingest_time = std::chrono::steady_clock::now();

    // For now we use the arrow name, but let's make this a lot more efficient
    SchedulerState& arrow_state = m_scheduler_state[task.arrow->get_name()];

    arrow_state.active_tasks -= 1;

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
};


void JExecutionEngine::FindNextReadyTask_Unsafe(Task& task, bool& found_task, bool& found_termination) {

    // First check to see if the topology itself has shut down
    if (m_runstatus != RunStatus::Running && m_runstatus != RunStatus::Draining) {

        // Because the topology is not running, there's nothing to do
        found_task = false;
        found_termination = false; 
        // This doesn't mean we should terminate, though, because we create our thread team _before_ we call Run().
        // Worker termination is handled through Scale() and needs a per-worker flag. We will have to add this later.

        task.arrow = nullptr;
        task.input_event = nullptr;
        task.input_port = -1;
        return;
    }

    // Because we reached this point, we know that the topology is running, 
    // so we look for work we can do
    for (auto& [arrow_name, state] : m_scheduler_state) {
        if (!state.is_parallel && (state.active_tasks != 0)) {
            // We've found a sequential arrow that is already running. Nothing we can do here.
            continue;
        }
        // TODO: Support next_visit_time so that we don't hammer blocked event sources

        // See if we can obtain an input event (this is silly)
        //JArrow* arrow = 
    }

    // Because we reached this point, we know that there aren't any tasks ready,
    // so we check whether more are potentially coming. If not, we can pause the topology.
    // Note that our worker threads will still wait at ExchangeTask() until they get
    // shut down separately during Scale().

    bool any_active_source_found = false;
    bool any_active_task_found = false;

    for (auto& [arrow_name, state] : m_scheduler_state) {
        bool active_source_found = (state.is_active && state.is_source);
        bool active_task_found = (state.active_tasks != 0);
        if (!active_source_found) {
            // Sanity check: no active tasks for inactive sources
            assert(!active_task_found);
        }
        any_active_source_found |= active_source_found;
        any_active_task_found |= active_task_found;
    }

    if (!any_active_source_found && !any_active_task_found) {
        // Pause the topology
        m_runstatus = RunStatus::Paused;
    }

    found_task = false;
    found_termination = false;
}
