
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <memory>

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "JTestParser.h"
#include "JTestPlotter.h"
#include "JTestPlotterLegacy.h"
#include "JTestDisentangler.h"
#include "JTestTracker.h"


extern "C"{
void InitPlugin(JApplication *app){

	InitJANAPlugin(app);
    app->Add(new JTestParser);
	app->Add(new JFactoryGeneratorT<JTestDisentangler>());
	app->Add(new JFactoryGeneratorT<JTestTracker>());

    bool order_output = app->RegisterParameter("jtest:order_output", false);

    bool use_legacy_plotter = app->RegisterParameter("jtest:use_legacy_plotter", false);
    if (use_legacy_plotter) {
        app->Add(new JTestPlotterLegacy);
    }
    else {
        app->Add(new JTestPlotter {order_output});
    }

    bool except_on_loading = app->RegisterParameter("jtest:except_on_loading", false);
    if (except_on_loading) {
        throw JException("Planned exception on loading!");
    }

    // Demonstrates sharing user-defined services with our components
	app->ProvideService(std::make_shared<JTestCalibrationService>());

}
} // "C"

