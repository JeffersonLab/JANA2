// Copyright 2026, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JApplication.h>
#include <JANA/JVersion.h>
#include <JANA/Services/JPerfettoService.h>

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->ProvideService(std::make_shared<JPerfettoService>());

    // Ensure that the call graph is recorded so that factory dependency chains
    // are captured alongside the Perfetto timeline.
    app->SetParameterValue("record_call_stack", 1);
}
} // extern "C"
