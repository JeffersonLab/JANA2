
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "BlockExampleSource.h"
#include "BlockExampleProcessor.h"


int main(int argc, char* argv[]) {

	JApplication app;
	app.Add(new BlockExampleProcessor);
	app.Run(true);
}

