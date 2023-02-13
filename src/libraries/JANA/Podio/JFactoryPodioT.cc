
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JFactoryPodioT.h"
#include <JANA/JEvent.h>

podio::Frame* GetFrame(const JEvent& event) {

    return const_cast<podio::Frame*>(event.GetSingle<podio::Frame>(""));
}
