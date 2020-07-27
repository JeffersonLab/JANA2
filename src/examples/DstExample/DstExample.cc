
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


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

