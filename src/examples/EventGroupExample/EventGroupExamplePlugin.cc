
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JApplication.h>
#include <JANA/JLogger.h>

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

        LOG << "Calling SubmitAndWait for events " << event_number-5 << ".." << event_number-1 << LOG_END;

        // SubmitAndWait causes producer to block until GroupedEventProcessor finishes
        evt_src->SubmitAndWait(event_batch);

        // After this point, all of the events in event_batch have been processed!
        LOG << "SubmitAndWait returned! Producer thread may now access results for events "
            << event_number-5 << ".." << event_number-1 << LOG_END;

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
