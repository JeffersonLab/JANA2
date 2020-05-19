

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "DstExampleProcessor.h"
#include "DstExampleSource.h"

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

    LOG << "Loading DstExample" << LOG_END;
    app->Add(new DstExampleProcessor);
    app->Add(new DstExampleSource("dummy", app));
}
}

