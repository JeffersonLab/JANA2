// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "MyFileReaderGenerator.h"
#include "MyFileWriter.h"
#include "MyTimesliceSplitter.h"
#include "MyProtoclusterFactory.h"
#include "MyClusterFactory.h"

#include <JANA/Omni/JOmniFactoryGeneratorT.h>

#include <JANA/JApplication.h>


extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);

    // Event source generator instantiates a FileReader for each filename passed to jana.
    // The event source it produces is configured to either produce Timeslices or Events.
    // Either way, these files contain just hits
    app->Add(new MyFileReaderGenerator());

    // Event processor that writes events (and timeslices, if they are present) to file
    app->Add(new MyFileWriter());

    // Unfolder that takes timeslices and splits them into physics events.
    app->Add(new MyTimesliceSplitter());

    // Factory that produces timeslice-level protoclusters from timeslice-level hits
    app->Add(new JOmniFactoryGeneratorT<MyProtoclusterFactory>(
                { .tag = "timeslice_protoclusterizer", 
                  .level = JEventLevel::Timeslice,
                  .input_names = {"hits"}, 
                  .output_names = {"ts_protoclusters"}
                }));

    // Factory that produces event-level protoclusters from event-level hits
    app->Add(new JOmniFactoryGeneratorT<MyProtoclusterFactory>(
                { .tag = "event_protoclusterizer", 
                  .input_names = {"hits"}, 
                  .output_names = {"evt_protoclusters"}}
                ));

    // Factory that produces event-level clusters from event-level protoclusters
    app->Add(new JOmniFactoryGeneratorT<MyClusterFactory>(
                { .tag = "clusterizer", 
                  .input_names = {"evt_protoclusters"}, 
                  .output_names = {"clusters"}}
                ));


}
} // "C"


