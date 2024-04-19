// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "MyFileReader.h"
#include "MyFileWriter.h"
#include "MyTimesliceSplitter.h"
#include "MyProtoclusterFactory.h"
#include "MyClusterFactory.h"

#include <JANA/Omni/JOmniFactoryGeneratorT.h>

#include <JANA/JApplication.h>


extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);


    // Unfolder that takes timeslices and splits them into physics events.
    app->Add(new MyTimesliceSplitter());

    // Factory that produces timeslice-level protoclusters from timeslice-level hits
    app->Add(new JOmniFactoryGeneratorT<MyProtoclusterFactory>(
                { .tag = "timeslice_protoclusterizer", 
                  .level = JEventLevel::Timeslice,
                  .input_tags = {"hits"}, 
                  .output_tags = {"ts_protoclusters"}},
                app));

    // Factory that produces event-level protoclusters from event-level hits
    app->Add(new JOmniFactoryGeneratorT<MyProtoclusterFactory>(
                { .tag = "event_protoclusterizer", 
                  .input_tags = {"hits"}, 
                  .output_tags = {"evt_protoclusters"}}, 
                app));

    // Factory that produces event-level clusters from event-level protoclusters
    app->Add(new JOmniFactoryGeneratorT<MyClusterFactory>(
                { .tag = "clusterizer", 
                  .input_tags = {"evt_protoclusters"}, 
                  .output_tags = {"clusters"}},
                app));

    // Event source that can read files containing either timeslices or events
    // Either way, these files contain just hits
    app->Add(new MyFileReader());

    // Processor that writes events (and timeslices, if they are present) to file
    app->Add(new MyFileWriter());

    app->SetParameterValue("jana:extended_report", 0);
}
} // "C"


