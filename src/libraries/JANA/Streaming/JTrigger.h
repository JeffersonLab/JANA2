
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

/// JTrigger determines whether an event contains data worth passing downstream, or whether
/// it should be immediately recycled. The user can call arbitrary JFactories from a Trigger
/// just like they can from an EventProcessor.
///
/// This design allows the user to reuse reconstruction code for a software trigger, and to
/// reuse results calculated for the trigger during reconstruction. The accept() function
/// should be thread safe, so that the trigger can be automatically parallelized, which will
/// help bound the system's overall latency.
///
/// Users should declare their accept() implementation as `final`, so that JANA can devirtualize it.

struct JTrigger {

    virtual bool accept(JEvent&) { return true; }

};


