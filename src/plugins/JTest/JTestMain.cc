
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <memory>

#include <JANA/JApplication.h>
#include <JANA/JCsvWriter.h>

#include "JTestParser.h"
#include "JTestPlotter.h"
#include "JTestDisentangler.h"
#include "JTestTracker.h"


extern "C"{
void InitPlugin(JApplication *app){

	InitJANAPlugin(app);
    app->Add(new JTestParser("dummy_source", app));
    app->Add(new JTestPlotter(app));
	app->Add(new JFactoryGeneratorT<JTestDisentangler>());
	app->Add(new JFactoryGeneratorT<JTestTracker>());

    // Demonstrates attaching a CSV writer so we can view the results from any JFactory
    app->SetParameterValue<std::string>("csv:dest_dir", ".");
    app->Add(new JCsvWriter<JTestTrackData>());

    // Demonstrates sharing user-defined services with our components
	app->ProvideService(std::make_shared<JTestCalibrationService>());

}
} // "C"

