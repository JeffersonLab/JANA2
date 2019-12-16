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

#include <JANA/JApplication.h>
#include "GroupedEventProcessor.h"
#include "GroupedEventSource.h"
#include "BlockingGroupedEventSource.h"


/// The producer thread generates and feeds TridasEvents to the BlockingEventSource.
void producer_thread(BlockingGroupedEventSource* evt_src, JApplication* app, int starting_event_number = 1) {

    int event_number = starting_event_number;
    std::vector<TridasEvent*> event_batch;

    for (int group_number = 1; group_number < 6; ++group_number) {

        // Emit groups of 5 TRIDAS events apiece

        for (int i=1; i<=5; ++i) {
            auto event = new TridasEvent;
            event->event_number = event_number++;
            event->run_number = 22;
            event->should_keep = false;
            event_batch.push_back(event);
        }

        // Note: In general, the producer really shouldn't be responsible for setting group number, because they
        // can really mess that up. For now we are just doing this so that we can easily see the 'barrier' in the logs

        std::cout << "Calling SubmitAndWait for events " << event_number-5 << ".." << event_number-1 << std::endl;

        // SubmitAndWait causes producer to block until GroupedEventProcessor finishes
        evt_src->SubmitAndWait(event_batch);

        // After this point, all of the events in event_batch have been processed!
        std::cout << "SubmitAndWait returned! Producer thread may now access results for events "
                  << event_number-5 << ".." << event_number-1 << std::endl;

        // Note: Just for demonstration purposes, we are setting should_keep=false in the producer and having JANA
        // set it to true in the GroupedEventProcessor. More realistically, a JFactory would make a decision.

        // Verify that JANA indeed processed all of the events, and that it didn't free the TridasEvents afterwards!
        for (int i=0; i<5; ++i) {
            assert(event_batch[i]->should_keep);
            delete event_batch[i];
        }
        event_batch.clear();
    }

    // Stop execution once producer is completely finished
    app->Quit();
}



extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);

    app->SetParameterValue("jana:extended_report", false);

    app->Add(new GroupedEventProcessor());

    auto evt_src = new BlockingGroupedEventSource("blocking_source", app);

    app->Add(evt_src);

    // Launch a separate thread which generates TRIDAS events and submits them to the event source
    new std::thread([=](){ producer_thread(evt_src, app); });

    // We can run multiple producer threads, which will correctly interleave execution within JANA
    new std::thread([=](){ producer_thread(evt_src, app, 100); });


}
} // "C"
