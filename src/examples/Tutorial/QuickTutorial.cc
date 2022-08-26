
#include <JANA/JEventProcessor.h>
#include <JANA/JCsvWriter.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/Services/JGlobalRootLock.h>

#include "RandomSource.h"
#include "Hit.h"
#include "Cluster_factory_Simple.h"
#include "TutorialProcessor.h"


extern "C" {
    void InitPlugin(JApplication *app) {
        InitJANAPlugin(app);
        app->Add(new TutorialProcessor);
        app->Add(new RandomSource("random", app));
//        app->Add(new JEventSourceGeneratorT<RandomSource>);
        app->Add(new JCsvWriter<Hit>);
        app->Add(new JFactoryGeneratorT<Cluster_factory_Simple>);
    }
}
    
