// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
//
// Implement a simple event processor. For the purposes of this
// example plugin, we Get the Cluster objects. Doing so tells
// JANA to activate the JFactory_Cluster::Process method so that
// the Cluster objects are created.
//
// Note that the "Cluster" objects produced inherit from TObject but nothing
// special is needed here to accommodate that.

#include "JTestRootProcessor.h"
#include <JANA/JLogger.h>

#include "Cluster.h"

JTestRootProcessor::JTestRootProcessor() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

void JTestRootProcessor::ProcessSequential(const JEvent& event) {
    // Get the cluster objects
    auto clusters = event.Get<Cluster>();

    // Lock mutex so operations on ROOT objects are serialized
    std::lock_guard<std::mutex>lock(m_mutex);

    // At this point we can do something with the Cluster objects
    // for (auto cluster : clusters) {
    //      //Use cluster object (e.g. fill histogram or write object to output file)
    // }
}
