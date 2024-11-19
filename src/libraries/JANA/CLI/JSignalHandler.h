
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

class JApplication;

/// JSignalHandler attaches signal handlers for USR1, USR2, and CTRL-C to a given JApplication instance.
namespace JSignalHandler {

void register_handlers(JApplication* app);

}; // namespace JSignalHandler

