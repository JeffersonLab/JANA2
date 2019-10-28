//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#include <JANA/Engine/JDebugProcessingController.h>
#include <JANA/JFactorySet.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

void JDebugProcessingController::run_worker() {
    LOG_INFO(m_logger) << "Running JDebugProcessingController" << LOG_END;
    auto evt_srces = m_component_manager->get_evt_srces();
    auto evt_procs = m_component_manager->get_evt_procs();

    auto event = std::make_shared<JEvent>();
    auto factory_set = new JFactorySet(m_component_manager->get_fac_gens());
    event->SetFactorySet(factory_set);
    event->SetJApplication(m_app);

    for (JEventProcessor* proc : evt_procs) {
        proc->DoInitialize(); // TODO: Bug with std::call_once?!?
    }

    // TODO: Assumes one event source
    bool more_events = true;
    size_t event_nr = 0;
    for (JEventSource* evt_src : evt_srces) {
        evt_src->DoInitialize();
        while (more_events && !m_stop_requested) {
            try {
                factory_set->Release();
                evt_src->GetEvent(event);
                //LOG_DEBUG(m_logger) << "Acquired event " << event->GetEventNumber() << " from " << evt_src->GetResourceName() << LOG_END;
                event->SetJEventSource(evt_src);
                event->SetEventNumber(++event_nr);
                // TODO: Assumes JEvtSrc doesn't throw kSUCCESS
                for (JEventProcessor* proc : evt_procs) {
                    //LOG_DEBUG(m_logger) << "Processing: " << proc->GetTypeName() << LOG_END;
                    proc->DoMap(event);
                }
                m_total_events_processed += 1;
            }
            catch (JEventSource::RETURN_STATUS rs) {
                switch (rs) {
                    case JEventSource::RETURN_STATUS::kNO_MORE_EVENTS :
                    case JEventSource::RETURN_STATUS::kERROR:
                    case JEventSource::RETURN_STATUS::kUNKNOWN:
                        //LOG_DEBUG(m_logger) << "Giving up on eventsource" << LOG_END;
                        more_events = false; break;

                    case JEventSource::RETURN_STATUS::kBUSY:
                    case JEventSource::RETURN_STATUS::kTRY_AGAIN:
                        //std::this_thread::yield();
                        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        LOG_TRACE(m_logger) << "EventSource is not ready yet" << LOG_END;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    LOG_INFO(m_logger) << "Finished JDebugProcessingController::run_worker" << LOG_END;
    for (JEventProcessor* evt_prc : evt_procs) {
        evt_prc->DoFinalize();
    }
    m_stop_achieved = true;
    m_finish_achieved = !more_events;
}


void JDebugProcessingController::initialize() {
}

void JDebugProcessingController::run(size_t nthreads) {
    m_worker = new std::thread(&JDebugProcessingController::run_worker, this);
}

void JDebugProcessingController::scale(size_t nthreads) {
    return;
}

void JDebugProcessingController::request_stop() {
    m_stop_requested = true;
}

void JDebugProcessingController::wait_until_stopped() {
    m_stop_requested = true;
    if (m_worker != nullptr) {
        m_worker->join();
    }
    m_worker = nullptr;
    m_stop_achieved = true;
}

bool JDebugProcessingController::is_stopped() {
    return m_stop_achieved;
}

bool JDebugProcessingController::is_finished() {
    return m_finish_achieved;
}

JDebugProcessingController::~JDebugProcessingController() {
    wait_until_stopped();
    if (m_worker != nullptr) {
        delete m_worker;
    }
    m_worker = nullptr;
}

void JDebugProcessingController::print_report() {
    auto ps = measure_performance();
    std::cout << *ps << std::endl;
}

void JDebugProcessingController::print_final_report() {
    print_report();
}

std::unique_ptr<const JPerfSummary> JDebugProcessingController::measure_performance() {

    auto ps = std::make_unique<JPerfSummary>();

    m_perf_metrics.summarize(*ps);
    ps->thread_count = 1;
    ps->monotonic_events_completed = m_total_events_processed;
    return ps;
}


