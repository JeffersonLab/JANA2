

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "DstExampleProcessor.h"
#include "DstExampleSource.h"
#include "DstExampleFactory.h"

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    LOG << "Loading DstExample" << LOG_END;
    app->Add(new DstExampleProcessor);
    app->Add(new DstExampleSource("dummy", app));
    app->Add(new JFactoryGeneratorT<DstExampleFactory>());
}
}

