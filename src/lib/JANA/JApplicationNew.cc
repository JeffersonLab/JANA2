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
// Authors: Nathan Brei
//

#include <unordered_set>

#include "JApplicationNew.h"
#include "JEventSourceManager.h"
#include "JEventSourceArrow.h"
#include "JEventProcessorArrow.h"

using EventQueue = Queue<std::shared_ptr<const JEvent>>;

JApplicationNew::JApplicationNew(JParameterManager* params, std::vector<std::string>* event_sources)
    : JApplication(params, event_sources)
{
    jout << "JApplicationNew::JApplicationNew" << std::endl;
}

void JApplicationNew::Initialize() {
    jout << "JApplicationNew::Initialize" << std::endl;

    if (_initialized) return;
    _initialized = true;

    AttachPlugins();
    _run_state = RunState::BeforeRun;

    std::deque<JEventSource*> sources;
    _eventSourceManager->CreateSources();
    _eventSourceManager->GetUnopenedJEventSources(sources);

    // Not sure why we have to do this, will figure out later
    // // Get factory generators from event sources
    std::deque<JEventSource*> sEventSources;
    _eventSourceManager->GetUnopenedJEventSources(sEventSources);
    std::unordered_set<std::type_index> sSourceTypes;
    for(auto sSource : sEventSources)
    {
        auto sTypeIndex = sSource->GetDerivedType();
        if(sSourceTypes.find(sTypeIndex) != std::end(sSourceTypes))
            continue; //same type as before: duplicate factories!

        auto sGenerator = sSource->GetFactoryGenerator();
        if(sGenerator != nullptr)
            _factoryGenerators.push_back(sGenerator);
    }

    // Assume the simplest possible topology for now, complicate later
    auto queue = new EventQueue();
    queue->set_threshold(500);  // JTest throughput increases with threshold size: WHY?
    _queues.push_back(queue);

    for (auto src : sources) {

        JArrow* arrow = new JEventSourceArrow(src->GetName(), src, queue, this);
        _arrows.push_back(arrow);
        _sources.push_back(arrow);
        // create arrow for sources. Don't open until arrow.activate() called
    }

    //auto finished_queue = new EventQueue();
    //_queues.push_back(finished_queue);

    auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr);
    _arrows.push_back(proc_arrow);

    // Receive notifications when sinks finish
    proc_arrow->attach_downstream(this);
    attach_upstream(proc_arrow);

    for (auto proc :_eventProcessors) {
        proc_arrow->add_processor(proc);
        _sinks.push_back(proc_arrow);
    }

    _nthreads = JCpuInfo::GetNumCpus();
    _pmanager->SetDefaultParameter("nthreads", _nthreads, "The total number of worker threads");

}

void JApplicationNew::Run() {
    jout << "JApplicationNew::Run" << std::endl;
    Initialize();
    Scale(_nthreads);

    set_active(true);
    _run_state = RunState::DuringRun;
    _start_time = jclock_t::now();
    mRunStartTime = std::chrono::system_clock::now();
    _last_time = _start_time;

    while (is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if( _ticker_on ) PrintStatus();
    }
    Stop(false);
}

void JApplicationNew::Scale(int nthreads) {

    if (_schedulers.empty()) {
        _schedulers.push_back(new JScheduler(_arrows, _nthreads));
    }

    size_t current_workers = _workers.size();
    while (current_workers < nthreads) {
        _workers.push_back(new JWorker(current_workers, _schedulers[0]));
        current_workers++;
    }
    for (int i=0; i<nthreads; ++i) {
        _workers.at(i)->start();
    };
    for (size_t i=nthreads; i<current_workers; ++i) {
        _workers.at(i)->request_stop();
    }
    for (size_t i=nthreads; i<current_workers; ++i) {
        _workers.at(i)->wait_for_stop();
    }
    _nthreads = nthreads;
}

void JApplicationNew::Quit(bool skip_join) {
    jout << "JApplicationNew::Quit" << std::endl;
    _skip_join = skip_join;
    _quitting = true;
    Stop(skip_join);
}

void JApplicationNew::Stop(bool skip_join) {
    jout << "JApplicationNew::Stop" << std::endl;
    for (JArrow* arrow : _arrows) {
        arrow->set_active(false);
    }
    for (JWorker* worker : _workers) {
        worker->request_stop();
    }
    if (!skip_join) {
        for (JWorker* worker : _workers) {
            worker->wait_for_stop();
        }
        if (_run_state == RunState::DuringRun) {
            // We shouldn't usually end up here because sinks notify us when
            // they deactivate, automatically calling JTopology::set_active(false),
            // which stops the clock as soon as possible.
            // However, if for unknown reasons nobody notifies us, we still want to change
            // run state in an orderly fashion. If we do end up here, though, our _stop_time
            // will be late, throwing our metrics off.
            _stop_time = jclock_t::now();
        }
        _run_state = RunState::AfterRun;
    }
}

void JApplicationNew::Resume() {
    jout << "JApplicationNew::Resume" << std::endl;
    // for each source, setactive to true
    //
}

void JApplicationNew::UpdateResourceLimits() {
    jout << "JApplicationNew::UpdateResourceLimits" << std::endl;
    mFactorySetPool.Set_ControlParams( _nthreads*2, 10 );
}

uint64_t JApplicationNew::GetNeventsProcessed() {
    uint64_t message_count = 0;
    for (JArrow* arrow : _sinks) {
        size_t arrow_message_count, v0;
        duration_t v1,v2,v3;
        arrow->get_metrics(arrow_message_count, v0,v1,v2,v3);
        message_count += arrow_message_count;
    }
    return message_count;
}

void JApplicationNew::PrintStatus() {
    //for (QueueBase* queue : _queues) {
    //    std::cout << queue->get_name() << ": " << queue->get_item_count() << std::endl;
    //}
    //std::cout << std::endl;
    //for (JArrow* arrow : _arrows) {
    //   auto summary = JTopology::ArrowStatus(arrow);
    //   std::cout << summary.arrow_name << ": " << summary.messages_completed << " (" << summary.thread_count << " threads)" << std::endl;
    //}

    std::stringstream ss;
    ss << "  " << GetNeventsProcessed() << " events processed  " << Val2StringWithPrefix( GetInstantaneousRate() ) << "Hz (" << Val2StringWithPrefix( GetIntegratedRate() ) << "Hz avg)             ";
    std::cout << ss.str() << std::endl;
    std::cout << std::endl;
}

void JApplicationNew::PrintFinalReport() {
    jout << "JApplicationNew::PrintFinalReport" << std::endl;
}

/// Abstraction-breaking methods we shouldn't be using and would like
/// to get rid of
JThreadManager* JApplicationNew::GetJThreadManager() const {
    return nullptr;
}

std::shared_ptr<JTask<void>> JApplicationNew::GetVoidTask() {
    throw;
}


/// Determine whether or not anything in our topology is running
bool JApplicationNew::is_active() {
    for (auto arrow : _arrows) {
        if (arrow->is_active()) {
            return true;
        }
    }
    return false;
}

/// Start processing by activating desired source arrows,
/// and stop the clock as soon as all of the arrows have completed
void JApplicationNew::set_active(bool active) {

    if (active && !is_active()) {
        for (auto arrow : _sources) {
            arrow->set_active(true);
            arrow->notify_downstream(true);
        }
    }
    else {
        if (_run_state == RunState::DuringRun) {
            _stop_time = jclock_t::now();
            _run_state = RunState::AfterRun;
        }
    }
}








