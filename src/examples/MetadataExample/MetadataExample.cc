

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "MetadataAggregator.h"
#include "RandomTrackSource.h"
#include "TrackSmearingFactory.h"

extern "C" {
void InitPlugin(JApplication* app) {

    // This code is executed when the plugin is attached.
    // It should always call InitJANAPlugin(app) first, and then do one or more of:
    //   - Read configuration parameters
    //   - Register JFactoryGenerators
    //   - Register JEventProcessors
    //   - Register JEventSourceGenerators (or JEventSources directly)
    //   - Register JServices

    InitJANAPlugin(app);

    LOG << "Loading MetadataExample" << LOG_END;
    app->Add(new RandomTrackSource("dummy", app));           // To generate random "tracks"
    app->Add(new JFactoryGeneratorT<TrackSmearingFactory>);  // To perturb tracks
    app->Add(new MetadataAggregator);                        // To collect perf data on track generation and smearing

}
}

