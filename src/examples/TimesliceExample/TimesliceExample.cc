// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "MyTimesliceSource.h"
#include "MyTimesliceUnfolder.h"
#include "MyEventProcessor.h"
#include "MyTimesliceFactory.h"
#include "MyEventFactory.h"

#include <JANA/JApplication.h>


extern "C"{
void InitPlugin(JApplication *app) {

    InitJANAPlugin(app);

    app->Add(new MyTimesliceSource("Dummy"));
    app->Add(new MyTimesliceUnfolder);
    app->Add(new MyEventProcessor);

    app->Add(new JFactoryGeneratorT<MyTimesliceFactory>());
    app->Add(new JFactoryGeneratorT<MyEventFactory>());

    app->SetParameterValue("jana:extended_report", 0);
}
} // "C"


