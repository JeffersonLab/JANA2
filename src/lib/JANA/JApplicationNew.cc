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

void JApplicationNew::Initialize() {


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

    _pmanager->SetDefaultParameter("nthreads", _nthreads, "The total number of worker threads");

}








