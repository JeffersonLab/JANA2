
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JArrowTopology.h"
#include "JEventProcessorArrow.h"
#include "JEventSourceArrow.h"

JArrowTopology::JArrowTopology() = default;

JArrowTopology::~JArrowTopology() {
    LOG_DEBUG(m_logger) << "JArrowTopology: Entering destructor" << LOG_END;
    // finish(); // We don't want to call finish() here in case there was an exception in JArrow::initialize(), finalize()

    for (auto arrow : arrows) {
        delete arrow;
    }
    for (auto queue : queues) {
        delete queue;
    }
}


