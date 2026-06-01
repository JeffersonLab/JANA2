// Copyright 2026, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Services/JPerfettoService.h>

#if JANA2_HAVE_PERFETTO

#include <fstream>
#include <sstream>

// Allocate static storage for the categories (must be in exactly one .cc file).
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

void JPerfettoService::Init() {
    auto params = GetApplication()->GetJParameterManager();
    params->SetDefaultParameter("jana:perfetto_output", m_output_file,
        "Output file path for the Perfetto trace (written on exit).");
    params->SetDefaultParameter("jana:perfetto_enabled", m_enabled,
        "Enable Perfetto tracing.");

    if (!m_enabled) return;

    perfetto::TracingInitArgs args;
    args.backends |= perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();

    perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(1024 * 256); // 256 MB ring buffer
    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    m_tracing_session = perfetto::Tracing::NewTrace();
    m_tracing_session->Setup(cfg);
    m_tracing_session->StartBlocking();

    LOG_INFO(GetLogger()) << "Perfetto tracing started. Output will be written to: " << m_output_file << LOG_END;
}

JPerfettoService::~JPerfettoService() {
    if (!m_enabled || !m_tracing_session) return;

    perfetto::TrackEvent::Flush();
    m_tracing_session->StopBlocking();

    std::vector<char> trace_data(m_tracing_session->ReadTraceBlocking());
    std::ofstream output(m_output_file, std::ios::out | std::ios::binary);
    if (!output) {
        std::cerr << "JPerfettoService: Failed to open Perfetto output file: " << m_output_file << "\n";
        return;
    }
    output.write(trace_data.data(), static_cast<std::streamsize>(trace_data.size()));
    std::cout << "JPerfettoService: Perfetto trace written to: " << m_output_file
              << " (" << trace_data.size() / 1024 << " kB)\n";
    std::cout << "JPerfettoService: Open the trace at https://ui.perfetto.dev\n";
}

void JPerfettoService::RegisterCurrentThread(int worker_id) {
    std::ostringstream name;
    name << "Worker " << worker_id;
    perfetto::ThreadTrack track = perfetto::ThreadTrack::Current();
    perfetto::protos::gen::TrackDescriptor desc = track.Serialize();
    desc.set_name(name.str());
    perfetto::TrackEvent::SetTrackDescriptor(track, desc);
}

#else // JANA2_HAVE_PERFETTO

void JPerfettoService::Init() {
    std::cerr << "JPerfettoService: JANA2 was not built with Perfetto support (USE_PERFETTO=OFF).\n";
}

JPerfettoService::~JPerfettoService() {}

void JPerfettoService::RegisterCurrentThread(int /*worker_id*/) {}

#endif // JANA2_HAVE_PERFETTO
