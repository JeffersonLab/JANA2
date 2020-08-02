
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JLogger.h>

#include "JControlZMQ.h"


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

    LOG << "Loading janacontrol" << LOG_END;

    int JANA_ZMQ_PORT = 11238;
    app->SetDefaultParameter("JANA_ZMQ_PORT", JANA_ZMQ_PORT, "TCP port used to by janacontrol plugin for ZMQ communication.");

    // TODO: Find some way of deleting this object at an appropriate time.
    new JControlZMQ( app, JANA_ZMQ_PORT );

}
}

