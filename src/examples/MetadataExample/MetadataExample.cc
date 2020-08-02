
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "MetadataAggregator.h"
#include "RandomTrackSource.h"
#include "TrackSmearingFactory.h"

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    LOG << "Loading MetadataExample" << LOG_END;
    app->Add(new RandomTrackSource("dummy", app));           // To generate random "tracks"
    app->Add(new JFactoryGeneratorT<TrackSmearingFactory>);  // To perturb tracks
    app->Add(new MetadataAggregator);                        // To collect perf data on track generation and smearing

}
}

