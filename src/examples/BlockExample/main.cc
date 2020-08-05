
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "BlockExampleSource.h"
#include "BlockExampleProcessor.h"

extern "C" {
void InitPlugin(JApplication* app) {

	InitJANAPlugin(app);

	LOG << "Loading BlockExample" << LOG_END;
	app->Add(new BlockExampleProcessor);
}
}

