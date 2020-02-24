

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JCsvWriter.h>

#include "TutorialProcessor.h"
#include "RandomSource.h"
#include "Hit.h"
#include "SimpleClusterFactory.h"

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    LOG << "Loading Tutorial" << LOG_END;
    app->Add(new TutorialProcessor);
    app->Add(new JCsvWriter<Hit>);
    app->Add(new JFactoryGeneratorT<SimpleClusterFactory>);

    app->Add(new RandomSource("random", app));            // Always use RandomSource
    //app->Add(new JEventSourceGeneratorT<RandomSource>); // Only use RandomSource when
                                                          //  'random' specified on cmd line

}
}

