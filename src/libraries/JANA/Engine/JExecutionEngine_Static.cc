
#include "JANA/Engine/JExecutionEngine_Static.h"
#include "JANA/JLogger.h"
#include "JANA/Topology/JArrow.h"
#include "JANA/Utils/JEventLevel.h"
#include <JANA/Utils/JApplicationInspector.h>
#include <JANA/JVersion.h>
#include <thread>

#if JANA2_HAVE_PERFETTO
#include <JANA/Services/JPerfettoService.h>
#endif

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

static thread_local JBacktrace* jana2_worker_backtrace = nullptr;

size_t JExecutionEngine_Static::CalculateMaxInflightEvents(JEventLevel level) {
    // The strategy is to be able to exactly fill all worker threads at the same time
    // This may not be optimal due to
    // - widely differing throughputs for different arrows
    // - load balancing problems due to widely varying event sizes
    // - memory limitations preventing scaling up to higher nthreads
    // but it is a starting point
    size_t result = 0;
    for (auto* arrow: m_topology->GetArrows()) {
        for (auto& port : arrow->GetAllPorts()) {
            // For now we assume each port has exactly one eventlevel
            // This will need to change if we ever have multi-eventlevel ports, e.g. for interleaved streams
            if (port->GetLevels().at(0) == level) {
                if (arrow->IsParallel()) {
                    result += m_requested_parallelism;
                }
                else {
                    result += 1;
                }
                // We want to count each arrow exactly once. Port direction and multiplicity don't matter
                break;
            }
        }
    }
    return result;
}

void JExecutionEngine_Static::Init() {
    auto params = GetApplication()->GetJParameterManager();

    // We parse the 'nthreads' parameter two different ways for backwards compatibility.
    m_requested_parallelism = 1;
    if (params->Exists("nthreads")) {
        if (params->GetParameterValue<std::string>("nthreads") == "Ncores") {
            m_requested_parallelism = JCpuInfo::GetNumCpus();
        } else {
            m_requested_parallelism = params->GetParameterValue<int>("nthreads");
        }
    }

    m_max_inflight_events[JEventLevel::Run] = params->RegisterParameter("jana:max_inflight_runs",
                                CalculateMaxInflightEvents(JEventLevel::Run),
                                "The number of runs which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Subrun] = params->RegisterParameter("jana:max_inflight_subruns",
                                CalculateMaxInflightEvents(JEventLevel::Subrun),
                                "The number of subruns which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Timeslice] = params->RegisterParameter("jana:max_inflight_timeslices",
                                CalculateMaxInflightEvents(JEventLevel::Timeslice),
                                "The number of timeslices which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Block] = params->RegisterParameter("jana:max_inflight_blocks",
                                CalculateMaxInflightEvents(JEventLevel::Block),
                                "The number of blocks which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::SlowControls] = params->RegisterParameter("jana:max_inflight_slowcontrols",
                                CalculateMaxInflightEvents(JEventLevel::SlowControls),
                                "The number of slow control events which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::PhysicsEvent] = params->RegisterParameter("jana:max_inflight_events",
                                CalculateMaxInflightEvents(JEventLevel::PhysicsEvent),
                                "The number of physics events which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Subevent] = params->RegisterParameter("jana:max_inflight_subevents",
                                CalculateMaxInflightEvents(JEventLevel::Subevent),
                                "The number of subevents which may be in-flight at once.");

    m_max_inflight_events[JEventLevel::Task] = params->RegisterParameter("jana:max_inflight_tasks",
                                CalculateMaxInflightEvents(JEventLevel::Task),
                                "The number of tasks which may be in-flight at once.");

    params->SetDefaultParameter("jana:timeout", m_timeout_s, 
        "Max time (in seconds) JANA will wait for a thread to update its heartbeat before hard-exiting. 0 to disable timeout completely.");

    params->SetDefaultParameter("jana:warmup_timeout", m_warmup_timeout_s, 
        "Max time (in seconds) JANA will wait for 'initial' events to complete before hard-exiting.");

    params->SetDefaultParameter("jana:backoff_interval", m_backoff_ms, 
        "Time (in ms) that workers will sleep when there is no work available");

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
    for (JArrow* arrow : m_topology->GetArrows()) {

        arrow->Initialize();

        m_arrow_states.emplace_back();
        auto& arrow_state = m_arrow_states.back();
        arrow_state.arrow_id = m_arrow_states.size() - 1;
        arrow_state.is_source = arrow->IsSource();
        arrow_state.is_sink = arrow->IsSink();
        arrow_state.is_parallel = arrow->IsParallel();
        arrow_state.next_input = arrow->GetNextPortIndex();
    }
    m_arrow_count = m_topology->GetArrows().size();

    // Scale all queues and pools, applying the user's max_inflight_* settings
    // Note that these values persist for the lifetime of the JApplication
    ScaleQueues();
}

void JExecutionEngine_Static::RequestInspector() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_interrupt_status = InterruptStatus::InspectRequested;
}

void JExecutionEngine_Static::RunTopology() {
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
        if (arrow.status == ArrowControlBlock::Status::Paused) {
            arrow.status = ArrowControlBlock::Status::Running;
        }
    }

    m_runstatus = RunStatus::Running;
    LOG_INFO(GetLogger()) << "Topology status changed to Running";

    lock.unlock();
    //m_condvar.notify_one();
}


void JExecutionEngine_Static::ScaleWorkers() {
    ScaleWorkers(m_requested_parallelism);
}


void JExecutionEngine_Static::ScaleWorkers(size_t requested_parallelism) {
    // We both create and destroy the pool of workers here. They all sleep until they 
    // receive work from the scheduler, which won't happen until the runstatus <- {Running, 
    // Pausing, Draining} and there is a task ready to execute. This way worker creation/destruction
    // is decoupled from topology execution.

    // If we scale to zero, no workers will run. This is useful for testing, and also for using
    // an external thread team, should the need arise.


    std::unique_lock<std::mutex> lock(m_mutex);

    if (requested_parallelism != 0 && m_arrow_states.size() == 0) {
        // We check that (nthreads != 0) because this gets called at shutdown even if the topology wasn't run
        // Remember, we want JApplication::Initialize() to succeed and JMain to shut down cleanly even when the topology is empty
        throw JException("Cannot execute an empty topology! Hint: Have you provided an event source?");
    }

    auto prev_parallelism = m_worker_parallelism;
    if (prev_parallelism == requested_parallelism) return;
    m_worker_parallelism = requested_parallelism;
    m_requested_parallelism = requested_parallelism;

    if (prev_parallelism < requested_parallelism) {
        // We are launching additional worker threads
        LOG_INFO(m_logger) << "Scaling up to " << requested_parallelism << " threads per parallel arrow." << LOG_END;

        auto mapping = m_topology->GetProcessorMapping();
        for (ArrowControlBlock as : m_arrow_states) {

            size_t worker_count_for_arrow = (as.is_parallel) ? requested_parallelism : 1;
            for (size_t i = prev_parallelism; i < worker_count_for_arrow; ++i) {

                auto worker = std::make_unique<WorkerControlBlock>();
                worker->worker_id = m_worker_states.size();
                worker->worker_nr = i;
                worker->arrow_id = as.arrow_id;
                worker->is_stop_requested = false;
                worker->cpu_id = mapping.get_cpu_id(i);
                worker->location_id = mapping.get_loc_id(i);

                LOG_DEBUG(GetLogger()) << "Launching worker thread " << worker->worker_id 
                    << " on arrow=" << m_topology->GetArrows().at(as.arrow_id)->GetName()
                    << ", cpu=" << worker->cpu_id << ", location=" << worker->location_id << LOG_END;

                worker->thread = new std::thread(&JExecutionEngine_Static::RunWorker, this, Worker{
                    .worker_id = worker->worker_id, 
                    .backtrace = &worker->backtrace,
                    .outputs = {}});
                bool pin_to_cpu = (mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
                if (pin_to_cpu) {
                    JCpuInfo::PinThreadToCpu(worker->thread, worker->cpu_id);
                }

                m_worker_states.push_back(std::move(worker));

            }
        }
    }
    else if (prev_parallelism > requested_parallelism) {
        LOG_INFO(m_logger) << "Scaling down to " << requested_parallelism << " threads per parallel arrow." << LOG_END;

        for (auto& ws : m_worker_states) {
            if (ws->worker_nr >= requested_parallelism) {
                LOG_DEBUG(GetLogger()) << "Stopping worker " << ws->worker_id 
                    << " (arrow = " << m_topology->GetArrows().at(ws->arrow_id)->GetName()
                    << ", worker_nr = " << ws->worker_nr << ")";
                ws->is_stop_requested = true;
            }
        }
        // m_condvar.notify_all(); // Wake up all threads so that they can exit the condvar wait loop
        lock.unlock();

        // We join all (eligible) threads _outside_ of the mutex
        for (auto& ws: m_worker_states) {
            if (ws->is_timed_out && ws->thread != nullptr) {
                // Thread has timed out. Rather than non-cooperatively killing it,
                // we relinquish ownership of it but remember that it was ours once and
                // is still out there, somewhere, biding its time
                ws->thread->detach();
                LOG_DEBUG(GetLogger()) << "Detached worker " << ws->worker_id << LOG_END;
            }
            else if (ws->is_stop_requested && ws->thread != nullptr) {
                LOG_DEBUG(GetLogger()) << "Joining worker " << ws->worker_id << LOG_END;
                ws->thread->join();
                LOG_DEBUG(GetLogger()) << "Joined worker " << ws->worker_id << LOG_END;
            }
        }

        lock.lock();
        for (auto& worker: m_worker_states) {
            if (worker->is_stop_requested) {
                // Delete the joined worker threads (so as to avoid leaking them)
                // but keep the WorkerControlBlock around, because removing it would
                // break worker_id corresponding to position in vector. This will
                // eventually cause a space leak if we repeatedly Scale() a long-running
                // process, but works fine for our purposes right now.
                delete worker->thread;
                worker->thread = nullptr;
            }
        }
    }
}

void JExecutionEngine_Static::ScaleQueues() {
    // The values of max_inflight_events were set one of two ways:
    // - In Init(), by obtaining a user parameter whose default value is set via CalculateMaxInflightEvents()
    // - In ScaleWorkers(requested_parallelism, autoscale_queues=true), by rerunning CalculateMaxInflightEvents() and ignoring user parameters
    // This lets us handle _three_ important cases:
    // 1. The user sets nthreads and possibly max_inflight_events parameters and runs as-is
    // 2. The user benchmarks over a range of nthreads using a single max_inflight_events map
    // 3. The user benchmarks over a range of nthreads using the max_inflight_events map calculated for each nthreads value
    
    size_t i=0;
    for (auto* pool: m_topology->GetPools()) {
        pool->Scale(m_max_inflight_events[pool->GetLevel()]);
        LOG_DEBUG(GetLogger()) << "Scaled pool " << i++ << " at level " << toString(pool->GetLevel()) << " to capacity=" << pool->GetCapacity();
    }
    for (auto* queue : m_topology->GetQueues()) {
        size_t capacity = 0;
        for (auto level : queue->GetLevels()) {
            capacity += m_max_inflight_events[level];
        }
        queue->Scale(capacity);
        LOG_DEBUG(GetLogger()) << "Scaled queue " << i++ << " to capacity=" << queue->GetCapacity();
    }
}

void JExecutionEngine_Static::PauseTopology() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_runstatus != RunStatus::Running) return;
    m_runstatus = RunStatus::Pausing;
    for (auto& arrow: m_arrow_states) {
        if (arrow.status == ArrowControlBlock::Status::Running) {
            arrow.status = ArrowControlBlock::Status::Paused;
        }
    }
    LOG_INFO(GetLogger()) << "Topology status changed to Pausing" << LOG_END;
    lock.unlock();
    m_condvar.notify_all();
}

void JExecutionEngine_Static::DrainTopology() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_runstatus != RunStatus::Running) return;
    m_runstatus = RunStatus::Draining;
    for (auto& arrow: m_arrow_states) {
        if (arrow.is_source) {
            if (arrow.status == ArrowControlBlock::Status::Running) {
                arrow.status = ArrowControlBlock::Status::Paused;
            }
        }
    }
    LOG_INFO(GetLogger()) << "Topology status changed to Draining" << LOG_END;
    lock.unlock();
    m_condvar.notify_all();
}

void JExecutionEngine_Static::RunSupervisor() {

    if (m_interrupt_status == InterruptStatus::NoInterruptsUnsupervised) {
        m_interrupt_status = InterruptStatus::NoInterruptsSupervised;
    }
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

    if (perf.runstatus == RunStatus::Failed) {
        HandleFailures();
    }
    PrintFinalReport();
}

bool JExecutionEngine_Static::CheckTimeout() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = clock_t::now();
    bool timeout_detected = false;
    for (auto& worker: m_worker_states) {
        auto timeout_s = (worker->is_event_warmed_up) ? m_timeout_s : m_warmup_timeout_s;
        auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(now - worker->last_checkout_time).count();
        if (duration_s > timeout_s && worker->has_task) {
            worker->is_timed_out = true;
            timeout_detected = true;
            m_runstatus = RunStatus::Failed;
        }
    }
    return timeout_detected;
}

void JExecutionEngine_Static::HandleFailures() {

    std::unique_lock<std::mutex> lock(m_mutex);

    // First, we log all of the failures we've found
    for (auto& worker: m_worker_states) {
        if (worker->is_timed_out && worker->thread != nullptr) {
            std::string arrow_name = (worker->arrow_id == static_cast<uint64_t>(-1)) ? "(none)" : m_topology->GetArrows()[worker->arrow_id]->GetName();
            LOG_FATAL(GetLogger()) << "Timeout in worker thread " << worker->worker_id << " while executing " << arrow_name << " on event #" << worker->last_event_nr << LOG_END;
            pthread_kill(worker->thread->native_handle(), SIGUSR2);
            LOG_INFO(GetLogger()) << "Worker thread signalled; waiting for backtrace capture." << LOG_END;
            worker->backtrace.WaitForCapture();
        }
        if (worker->stored_exception != nullptr) {
            std::string arrow_name = (worker->arrow_id == static_cast<uint64_t>(-1)) ? "(none)" : m_topology->GetArrows()[worker->arrow_id]->GetName();
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

void JExecutionEngine_Static::FinishTopology() {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(m_runstatus == RunStatus::Paused);

    LOG_DEBUG(GetLogger()) << "Finishing processing..." << LOG_END;
    for (auto* arrow : m_topology->GetArrows()) {
        arrow->Finalize();
    }
    for (auto* pool: m_topology->GetPools()) {
        pool->Finalize();
    }
    m_runstatus = RunStatus::Finished;
    LOG_INFO(GetLogger()) << "Topology status changed to Finished" << LOG_END;
}

JExecutionEngine::RunStatus JExecutionEngine_Static::GetRunStatus() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_runstatus;
}

JExecutionEngine::Perf JExecutionEngine_Static::GetPerf() {
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
    result.thread_count = m_worker_parallelism;
    result.throughput_hz = (result.uptime_ms == 0) ? 0 : (result.event_count * 1000.0) / result.uptime_ms;
    result.event_level = JEventLevel::PhysicsEvent;
    return result;
}

JExecutionEngine_Static::Worker JExecutionEngine_Static::RegisterWorker(std::string arrow_name) {
    std::unique_lock<std::mutex> lock(m_mutex);

    auto arrow = m_topology->GetArrow(arrow_name);

    auto mapping = m_topology->GetProcessorMapping();
    auto worker_id = m_worker_states.size();
    auto worker_state = std::make_unique<WorkerControlBlock>();
    worker_state->arrow_id = arrow->GetId();
    worker_state->worker_id = worker_id;
    worker_state->is_stop_requested = false;
    worker_state->cpu_id = mapping.get_cpu_id(worker_id);
    worker_state->location_id = mapping.get_loc_id(worker_id);
    worker_state->thread = nullptr;
    m_worker_states.push_back(std::move(worker_state));

    bool pin_to_cpu = (mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
    if (pin_to_cpu) {
        JCpuInfo::PinThreadToCpu(worker_state->thread, worker_state->cpu_id);
    }
    return {.worker_id=worker_id, .backtrace=&worker_state->backtrace, .block_until_next_task=false, .arrow=arrow, .outputs={}};
}


void JExecutionEngine_Static::RunWorker(Worker worker) {

    LOG_DEBUG(GetLogger()) << "Launched worker thread " << worker.worker_id << LOG_END;
#if JANA2_HAVE_PERFETTO
    JPerfettoService::RegisterCurrentThread(worker.worker_id);
#endif
    try {
        while (!worker.stop_requested) {
            ExchangeTask(worker);
            if (worker.has_active_task) {
#if JANA2_HAVE_PERFETTO
                TRACE_EVENT("jana", perfetto::DynamicString{task.arrow->GetName()},
                    "worker_id", (uint64_t)worker.worker_id);
#endif
                worker.arrow->Fire(worker.event, worker.outputs, worker.output_count, worker.output_status);
            }
        }
        LOG_DEBUG(GetLogger()) << "Stopped worker thread " << worker.worker_id << LOG_END;
    }
    catch(JException& ex) {
        LOG_ERROR(GetLogger()) << "Exception on worker thread " << worker.worker_id << ": " << ex.GetMessage();
        std::unique_lock<std::mutex> lock(m_mutex);
        m_runstatus = RunStatus::Failed;
        m_worker_states.at(worker.worker_id)->stored_exception = std::current_exception();
        LOG_INFO(GetLogger()) << "Topology status changed to Failed";
    }
    catch (...) {
        LOG_ERROR(GetLogger()) << "Exception on worker thread " << worker.worker_id << LOG_END;
        std::unique_lock<std::mutex> lock(m_mutex);
        m_runstatus = RunStatus::Failed;
        m_worker_states.at(worker.worker_id)->stored_exception = std::current_exception();
        LOG_INFO(GetLogger()) << "Topology status changed to Failed";
    }
}


void JExecutionEngine_Static::ExchangeTask(Worker& worker) {

    auto checkin_time = clock_t::now();
    // It's important to start measuring this _before_ acquiring the lock because acquiring the lock
    // may be a big part of the scheduler overhead

    std::unique_lock<std::mutex> lock(m_mutex);
    auto& worker_state = *m_worker_states.at(worker.worker_id);

    if (worker.output_status != JArrow::FireResult::NotRunYet) {
        ReturnResult_Unsafe(worker, checkin_time);
    }

    DetectPause_Unsafe();

    if (worker_state.is_stop_requested) {
        LOG_DEBUG(GetLogger()) << "Scheduler is passing stop_requested on to worker";
        worker.stop_requested = true;
        return;
    }

    FindNextReadyTask_Unsafe(worker);

    if (!worker.block_until_next_task) { return; }
    auto idle_time_start = clock_t::now();
    m_total_scheduler_duration += (idle_time_start - checkin_time);

    while (!worker_state.has_task && !worker_state.is_stop_requested) {
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(m_backoff_ms));
        lock.lock();
        //m_condvar.wait_for(lock, std::chrono::milliseconds(m_backoff_ms));
        DetectPause_Unsafe();
        FindNextReadyTask_Unsafe(worker);
    }

    if (worker_state.is_stop_requested) {
        LOG_DEBUG(GetLogger()) << "Scheduler is passing stop_requested on to worker";
        worker.stop_requested = true;
    }

    if (worker.event != nullptr) {
        worker_state.last_event_nr = worker.event->GetEventNumber();
    }
    else {
        worker_state.last_event_nr = -1;
    }
    worker_state.last_checkout_time = clock_t::now();
    m_total_idle_duration += (worker_state.last_checkout_time - idle_time_start);
}

void JExecutionEngine_Static::ReturnResult_Unsafe(Worker& worker, clock_t::time_point checkin_time) {

    auto& worker_state = *m_worker_states[worker.worker_id];
    worker_state.has_task = false;
    worker.has_active_task = false;

    ArrowControlBlock& arrow_state = m_arrow_states.at(worker_state.arrow_id);
    arrow_state.active_tasks -= 1;
    arrow_state.total_processing_duration += (checkin_time - worker_state.last_checkout_time);

    for (size_t output=0; output<worker.output_count; ++output) {
        // TODO: Model ports, events_processed, and event levels better
        //       Eventually get rid of result.arrow?
        if (!worker.arrow->GetPort(worker.outputs[output].second).GetSkipFinishEvent()) {
            arrow_state.events_processed++;
        }
    }

    // Put each output in its correct queue or pool
    worker.arrow->Push(worker.outputs, worker.output_count, worker_state.location_id);

    if (worker.output_status == JArrow::FireResult::Finished) {
        // If this is an eventsource self-terminating (the only thing that returns Status::Finished right now) it will
        // have already called DoClose(). I'm tempted to always call DoClose() as part of JExecutionEngine::Finish() instead, however.

        // Mark arrow as finished
        arrow_state.status = ArrowControlBlock::Status::Finished;

        // Check if this switches the topology to Draining()
        if (m_runstatus == RunStatus::Running) {
            bool draining = true;
            for (auto& arrow: m_arrow_states) {
                if (arrow.is_source && arrow.status == ArrowControlBlock::Status::Running) {
                    draining = false;
                }
            }
            if (draining) {
                m_runstatus = RunStatus::Draining;
                LOG_INFO(GetLogger()) << "Topology status changed to Draining";
            }
        }
    }

    worker.output_status = JArrow::FireResult::NotRunYet;
    worker.output_count = 0;
};

bool JExecutionEngine_Static::FindNextReadyTask_Unsafe(Worker& worker) {

    auto& worker_state = *m_worker_states[worker.worker_id];
    auto& arrow_state = m_arrow_states[worker_state.arrow_id];
    JArrow* arrow = m_topology->GetArrows()[worker_state.arrow_id];
    auto port = arrow->GetNextPortIndex(); // TODO: Not worker.input_port?

    worker.arrow = arrow;
    worker.event = nullptr;
    worker.has_active_task = false;

    if (m_runstatus != RunStatus::Running && m_runstatus != RunStatus::Draining) {
        // We only pick up a new task if the topology is running or draining.
        LOG_TRACE(GetLogger()) << "Worker " << worker.worker_id << " (" << arrow->GetName() << "): No tasks found: Topology status is not Running or Draining";
        return false;
    }
    else if (arrow_state.status != ArrowControlBlock::Status::Running) {
        LOG_TRACE(GetLogger()) << "Worker " << worker.worker_id << " (" << arrow->GetName() << "): No tasks found: Arrow status is not Running";
        return false;
    }
    else if (!arrow_state.is_parallel && (arrow_state.active_tasks != 0)) {
        // We've found a sequential arrow that is already active. Nothing we can do here.
        LOG_TRACE(GetLogger()) << "Worker " << worker.worker_id << " (" << arrow->GetName() << "): No tasks found: Arrow is sequential and a task is already active";
        return false;
    }
    else if (port == -1) {
        worker_state.has_task = true;
        arrow_state.active_tasks += 1;
        worker.has_active_task = true;
        worker_state.is_event_warmed_up = true; // Use shorter timeout
        worker_state.last_event_nr = 0;
        LOG_TRACE(GetLogger()) << "Worker " << worker.worker_id << " (" << arrow->GetName() << "): Task found: No event needed";
    }
    else {
        worker.event = arrow->Pull(port, worker_state.location_id);
        if (worker.event != nullptr) {
            worker.has_active_task = true;
            arrow_state.active_tasks += 1;
            worker_state.has_task = true;
            worker_state.is_event_warmed_up = worker.event->IsWarmedUp();
            worker_state.last_event_nr = worker.event->GetEventNumber();
            LOG_TRACE(GetLogger()) << "Worker " << worker.worker_id << " (" << arrow->GetName() << "): Task found: Event #" << worker.event->GetEventNumber() << " from port " << arrow->GetPort(port).GetName();
        }
        else {
            LOG_TRACE(GetLogger()) << "Worker " << worker.worker_id << " (" << arrow->GetName() << "): No tasks found: Queue empty on port " << arrow->GetPort(port).GetName();
        }
    }
    return false;
}



void JExecutionEngine_Static::DetectPause_Unsafe() {
    // Note that our worker threads will still wait at ExchangeTask() until they get
    // shut down separately during Scale().

    if (m_runstatus == RunStatus::Pausing || m_runstatus == RunStatus::Draining) {
        // We want to avoid scenarios such as where the topology already Finished but then gets reset to Paused
        // This also leaves a cleaner narrative in the logs. 

        LOG_DEBUG(GetLogger()) << "Scheduler: Checking if processing has paused" << LOG_END;

        for (auto& arrow : m_arrow_states) {
            if (arrow.status == ArrowControlBlock::Status::Running && arrow.is_source) {
                return;
            }
            if (arrow.active_tasks != 0) {
                return;
            }
        }

        if (m_runstatus == RunStatus::Draining) {
            // If we are pausing, it's okay to leave events in the queues
            // If we are draining, it's not
            // Note that this logic will leave _some_ events behind iff they are 'unready',
            //     i.e. in a queue attached to a port other than next_input_port
            //     This is acceptable because the alternative is that Drain() would fail to terminate.
            //     Also, draining isn't perfect: events might also be left in the arrows as well,
            //     particularly parent events. I'm considering adding an extra callback to UnfoldArrow
            //     in order to tell it when it can eject the parent. However this would be difficult
            //     to implement because arrows would have to be drained in topological order

            for (auto* arrow : m_topology->GetArrows()) {
                auto loc_count = m_topology->GetProcessorMapping().get_loc_count();
                for (size_t location = 0; location < loc_count; ++location) {

                    auto port_index = arrow->GetNextPortIndex();
                    if (port_index != -1) {
                        auto* queue = arrow->GetPort(port_index).GetQueue();
                        if (queue != nullptr && queue->GetSize(location) != 0) {
                            return;
                        }
                    }
                }
            }
        }

        // Pause the topology
        m_time_at_finish = clock_t::now();
        m_event_count_at_finish = 0;
        for (auto& arrow_state : m_arrow_states) {
            if (arrow_state.is_sink) {
                m_event_count_at_finish += arrow_state.events_processed;
            }
        }
        LOG_INFO(GetLogger()) << "Topology status changed to Paused" << LOG_END;
        m_runstatus = RunStatus::Paused;
    }
}


void JExecutionEngine_Static::PrintFinalReport() {

    std::unique_lock<std::mutex> lock(m_mutex);
    auto event_count = m_event_count_at_finish - m_event_count_at_start;
    auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_time_at_finish - m_time_at_start).count();
    auto thread_count = m_worker_parallelism;
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
        auto* arrow = m_topology->GetArrows()[arrow_id];
        auto& arrow_state = m_arrow_states[arrow_id];
        auto useful_ms = std::chrono::duration_cast<std::chrono::milliseconds>(arrow_state.total_processing_duration).count();
        total_useful_ms += useful_ms;
        auto avg_latency = useful_ms*1.0/arrow_state.events_processed;
        auto throughput_bottleneck = 1000.0 / avg_latency;
        if (arrow->IsParallel()) {
            throughput_bottleneck *= thread_count;
        }

        LOG_INFO(GetLogger()) << "  - Arrow name:                 " << arrow->GetName() << LOG_END;
        LOG_INFO(GetLogger()) << "    Parallel:                   " << arrow->IsParallel() << LOG_END;
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

void JExecutionEngine_Static::SetTickerEnabled(bool show_ticker) {
    m_show_ticker = show_ticker;
}

bool JExecutionEngine_Static::IsTickerEnabled() const {
    return m_show_ticker;
}

void JExecutionEngine_Static::SetTimeoutEnabled(bool timeout_enabled) {
    m_enable_timeout = timeout_enabled;
}

bool JExecutionEngine_Static::IsTimeoutEnabled() const {
    return m_enable_timeout;
}

JArrow::FireResult JExecutionEngine_Static::Fire(size_t arrow_id, size_t location_id) {

    std::unique_lock<std::mutex> lock(m_mutex);
    if (arrow_id >= m_topology->GetArrows().size()) {
        LOG_WARN(GetLogger()) << "Firing unsuccessful: No arrow exists with id=" << arrow_id << LOG_END;
        return JArrow::FireResult::NotRunYet;
    }
    JArrow* arrow = m_topology->GetArrows()[arrow_id];
    LOG_WARN(GetLogger()) << "Attempting to fire arrow with name=" << arrow->GetName() 
                          << ", index=" << arrow_id << ", location=" << location_id << LOG_END;

    ArrowControlBlock& arrow_state = m_arrow_states[arrow_id];
    if (arrow_state.status == ArrowControlBlock::Status::Finished) {
        LOG_WARN(GetLogger()) << "Firing unsuccessful: Arrow status is Finished." << arrow_id << LOG_END;
        return JArrow::FireResult::Finished;
    }
    if (!arrow_state.is_parallel && arrow_state.active_tasks != 0) {
        LOG_WARN(GetLogger()) << "Firing unsuccessful: Arrow is sequential and already has an active task." << arrow_id << LOG_END;
        return JArrow::FireResult::NotRunYet;
    }
    arrow_state.active_tasks += 1;

    auto port = arrow->GetNextPortIndex();
    JEvent* event = nullptr;
    if (port != -1) {
        event = arrow->Pull(port, location_id);
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
    arrow->Fire(event, outputs, output_count, result);
    LOG_WARN(GetLogger()) << "Fired arrow with result " << ToString(result) << LOG_END;
    if (output_count == 0) {
        LOG_WARN(GetLogger()) << "No output events" << LOG_END;
    }
    else {
        for (size_t i=0; i<output_count; ++i) {
            LOG_WARN(GetLogger()) << "Output event #" << outputs.at(i).first->GetEventNumber() << " on port " << outputs.at(i).second << LOG_END;
        }
    }

    lock.lock();
    arrow->Push(outputs, output_count, location_id);
    arrow_state.active_tasks -= 1;
    lock.unlock();
    return result;
}


void JExecutionEngine_Static::HandleSIGINT() {
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

void JExecutionEngine_Static::HandleSIGUSR1() {
    m_send_worker_report_requested = true;
}

void JExecutionEngine_Static::HandleSIGUSR2() {
    if (jana2_worker_backtrace != nullptr) {
        jana2_worker_backtrace->Capture(3);
    }
}

void JExecutionEngine_Static::HandleSIGTSTP() {
    std::cout << std::endl;
    m_print_worker_report_requested = true;
}

void JExecutionEngine_Static::PrintWorkerReport(bool send_to_pipe) {

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
            << "  Current arrow: " << worker->arrow_id << std::endl
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


