// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JLogger.h>

#include "JControlZMQ.h"


extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    int JANA_ZMQ_PORT = 11238;
    app->SetDefaultParameter("jana:zmq_port", JANA_ZMQ_PORT, "TCP port used to by janacontrol plugin for ZMQ communication.");

    LOG << "Loading janacontrol. Listening on port " << JANA_ZMQ_PORT << " (use jana:zmq_port to change)" << LOG_END;

    // TODO: Find some way of deleting this object at an appropriate time.
    new JControlZMQ( app, JANA_ZMQ_PORT );

}
}

