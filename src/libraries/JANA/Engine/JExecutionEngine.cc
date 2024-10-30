
#include "JExecutionEngine.h"
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
    assert(m_runstatus == RunStatus::Running | m_runstatus == RunStatus::Pausing || m_runstatus == RunStatus::Draining);
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
    std::optional<Task> task;
    try {
        do {
            task = ExchangeTask(task);
            worker.last_checkin_time = clock_t::now();
            // Do the work
        } while (task != std::nullopt);
    }
    catch (JException& e) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_runstatus = RunStatus::Failed;
        worker.stored_exception = e;
    }
}


std::optional<JExecutionEngine::Task> JExecutionEngine::ExchangeTask(std::optional<Task> completed_task) {

    std::unique_lock<std::mutex> lock(m_mutex);
    if (completed_task != std::nullopt) {
        FinishTaskUnsafe(*completed_task);
    }

    std::optional<Task> task = FindNextReadyTaskUnsafe();
    bool done = m_runstatus == RunStatus::Running || m_runstatus == RunStatus::Draining;

    while (task == std::nullopt && !done) {
        m_condvar.wait(lock);
        task = FindNextReadyTaskUnsafe();
        done = m_runstatus != RunStatus::Running && m_runstatus != RunStatus::Draining;
    }

    if (done) {
        return std::nullopt; // Tells worker to shut down
    }

    lock.unlock();
    // Notify one worker, who will notify the next, etc, as long as FindNextReadyTaskUnsafe() succeeds.
    // After FindNextReadyTaskUnsafe fails, all threads block until the next returning worker reactivates the
    // notification chain.
    m_condvar.notify_one();
    return task;
}

void JExecutionEngine::FinishTaskUnsafe(Task finished_task) {
    // Check if this is a source which has finished
    // If so, deactivate it
    // Check if this changes the run status to Paused
}

std::optional<JExecutionEngine::Task> JExecutionEngine::FindNextReadyTaskUnsafe() {
    // Iterate over each arrow
    // Find active input index
    // Check corresponding place
    // If present, 
    return Task{};
}


