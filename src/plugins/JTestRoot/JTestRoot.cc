// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
//
// This file is what makes this a plugin. When attached to a JANA process
// it will add a JTestRootEventSource, JTestRootProcessor and a JFactory_Cluster
// factory. The JFactory_Cluster will make Cluster objects that can also be used
// by other plugins.

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "JTestRootEventSource.h"
#include "JTestRootProcessor.h"
#include "JFactory_Cluster.h"

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    LOG << "Loading JTestRoot" << LOG_END;
    app->Add(new JTestRootEventSource);
    app->Add(new JTestRootProcessor);
    app->Add(new JFactoryGeneratorT<JFactory_Cluster>);
}
}

