
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "TutorialProcessor.h"
#include "RandomSource.h"
#include "SimpleClusterFactory.h"

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    LOG << "Loading Tutorial" << LOG_END;
    app->Add(new TutorialProcessor);
    app->Add(new JFactoryGeneratorT<SimpleClusterFactory>);

    // Always use RandomSource
    app->Add(new RandomSource("random", app));

    // Only use RandomSource when 'random' specified on cmd line
    // app->Add(new JEventSourceGeneratorT<RandomSource>); 

}
}

