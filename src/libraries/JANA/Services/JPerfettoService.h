// Copyright 2026, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JVersion.h>
#include <JANA/JService.h>

#if JANA2_HAVE_PERFETTO
#include <perfetto.h>

// Declare categories in a header so all TUs that use TRACE_EVENT share the same registry.
PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("jana").SetDescription("JANA2 framework events (arrow dispatch)"),
    perfetto::Category("factory").SetDescription("JANA2 factory Process() execution per event"),
    perfetto::Category("factory_init").SetDescription("JANA2 factory Init/ChangeRun/BeginRun/EndRun callbacks")
);
#endif // JANA2_HAVE_PERFETTO

class JPerfettoService : public JService {

public:
    JPerfettoService() = default;
    ~JPerfettoService() override;

    void Init() override;

    /// Call from each worker thread at startup to register the thread track with its worker ID.
    static void RegisterCurrentThread(int worker_id);

private:
#if JANA2_HAVE_PERFETTO
    std::unique_ptr<perfetto::TracingSession> m_tracing_session;
#endif
    std::string m_output_file = "jana_trace.perfetto";
    bool m_enabled = true;
};
