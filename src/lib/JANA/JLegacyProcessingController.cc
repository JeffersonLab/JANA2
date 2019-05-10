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

#include <JANA/JLegacyProcessingController.h>

#include <JANA/JEventProcessor.h>
#include "JLegacyProcessingController.h"


JLegacyProcessingController::JLegacyProcessingController(JApplication* app, JProcessingTopology* topology) {
    _threadManager = new JThreadManager(app);
    _topology = topology;
    _params = app->GetJParameterManager();
}

JLegacyProcessingController::~JLegacyProcessingController() {
    delete _threadManager;
    _threadManager = nullptr;
}

void JLegacyProcessingController::initialize() {

    _topology->event_source_manager.OpenInitSources();

    try {
        _params->SetDefaultParameter("AFFINITY", _affinity_algorithm, "Set the thread affinity algorithm. 0=none, 1=sequential, 2=core fill");
        _threadManager->SetThreadAffinity(_affinity_algorithm);
    }
    catch(...) {
        LOG_ERROR(_logger) << "Unknown exception in JApplication::Run attempting to set thread affinity" << LOG_END;
    }

    _threadManager->PrepareQueues();

}

void JLegacyProcessingController::run(size_t nthreads) {
    LOG_INFO(_logger) << "Creating " << _nthreads << " processing threads ..." << LOG_END;
    _nthreads = nthreads;
    _topology->set_active(true);
    _threadManager->CreateThreads(_nthreads);
    _threadManager->RunThreads();
}

void JLegacyProcessingController::scale(size_t nthreads) {
    _nthreads = nthreads;
    _threadManager->SetNJThreads(nthreads);
}

void JLegacyProcessingController::request_stop() {
    _threadManager->StopThreads(false);
}

void JLegacyProcessingController::wait_until_stopped() {
    _threadManager->StopThreads(true);
    _threadManager->JoinThreads();
    _threadManager->EndThreads();

    // Delete event processors
    for(auto sProcessor : _topology->event_processors){
        sProcessor->Finish(); // (this may not be necessary since it is always next to destructor)
        delete sProcessor;
    }
    _topology->event_processors.clear();
}

JThreadManager* JLegacyProcessingController::get_threadmanager() {
    return _threadManager;
}

void JLegacyProcessingController::print_report() {
    // If we want to present impl-specific information about how JLegacyProcessingController is performing,
    // we can do this here.
    auto perf = measure_performance();
    jout << *perf;
}

void JLegacyProcessingController::print_final_report() {

    //Get queues
    std::vector<JThreadManager::JEventSourceInfo*> sRetiredQueues;
    _threadManager->GetRetiredSourceInfos(sRetiredQueues);
    std::vector<JThreadManager::JEventSourceInfo*> sActiveQueues;
    _threadManager->GetActiveSourceInfos(sActiveQueues);
    auto sAllQueues = sRetiredQueues;
    sAllQueues.insert(sAllQueues.end(), sActiveQueues.begin(), sActiveQueues.end());


    // Get longest JQueue name
    uint32_t sSourceMaxNameLength = 0, sQueueMaxNameLength = 0;
    for(auto& sSourceInfo : sAllQueues)
    {
        auto sSource = sSourceInfo->mEventSource;
        auto sSourceLength = sSource->GetName().size();
        if(sSourceLength > sSourceMaxNameLength)
            sSourceMaxNameLength = sSourceLength;

        std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sSourceQueues;
        sSourceInfo->mQueueSet->GetQueues(sSourceQueues);
        for(auto& sTypePair : sSourceQueues)
        {
            for(auto sQueue : sTypePair.second)
            {
                auto sLength = sQueue->GetName().size();
                if(sLength > sQueueMaxNameLength)
                    sQueueMaxNameLength = sLength;
            }
        }
    }
    sSourceMaxNameLength += 2;
    if(sSourceMaxNameLength < 8) sSourceMaxNameLength = 8;
    sQueueMaxNameLength += 2;
    if(sQueueMaxNameLength < 7) sQueueMaxNameLength = 7;

    jout << std::endl;
    jout << "Final Report" << std::endl;
    jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
    jout << "Source" << std::string(sSourceMaxNameLength - 6, ' ') << "   Nevents  " << "Queue" << std::string(sQueueMaxNameLength - 5, ' ') << "NTasks" << std::endl;
    jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
    std::size_t sSrcIdx = 0;
    for(auto& sSourceInfo : sAllQueues)
    {
        // Flag to prevent source name and number of events from
        // printing more than once.
        bool sSourceNamePrinted = false;

        // Place "*" next to names of active sources
        string sFlag;
        if( sSrcIdx++ >= sRetiredQueues.size() ) sFlag="*";

        auto sSource = sSourceInfo->mEventSource;
        std::map<JQueueSet::JQueueType, std::vector<JQueue*>> sSourceQueues;
        sSourceInfo->mQueueSet->GetQueues(sSourceQueues);
        for(auto& sTypePair : sSourceQueues)
        {
            for(auto sQueue : sTypePair.second)
            {
                string sSourceName = sSource->GetName()+sFlag;

                if( sSourceNamePrinted ){
                    jout << string(sSourceMaxNameLength + 12, ' ');
                }else{
                    sSourceNamePrinted = true;
                    jout << sSourceName << string(sSourceMaxNameLength - sSourceName.size(), ' ')
                         << std::setw(10) << sSource->GetNumEventsProcessed() << "  ";
                }

                jout << sQueue->GetName() << string(sQueueMaxNameLength - sQueue->GetName().size(), ' ')
                     << sQueue->GetNumTasksProcessed() << std::endl;
            }
        }
    }
    if( !sActiveQueues.empty() ){
        jout << std::endl;
        jout << "(*) indicates sources that were still active" << std::endl;
    }
    jout << std::string(sSourceMaxNameLength + 12 + sQueueMaxNameLength + 9, '-') << std::endl;
}

bool JLegacyProcessingController::is_stopped() {
    return _threadManager->AreAllThreadsEnded();
}

std::unique_ptr<const JPerfSummary> JLegacyProcessingController::measure_performance() {
    _perf_summary.thread_count = _nthreads;
    _perf_summary.monotonic_events_completed = _topology->event_source_manager.GetNumEventsProcessed();

    return std::unique_ptr<JPerfSummary>(new JPerfSummary(_perf_summary));
}

