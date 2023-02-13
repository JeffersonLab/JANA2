
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "ExampleClusterFactory.h"
#include "datamodel/ExampleHit.h"
#include <JANA/JEvent.h>

void ExampleClusterFactory::Process(const std::shared_ptr<const JEvent> &event) {
    auto hits = event->GetCollection<ExampleHit>("hits");
}
