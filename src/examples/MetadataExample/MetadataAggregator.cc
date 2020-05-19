
#include "MetadataAggregator.h"
#include "Track.h"
#include "TrackMetadata.h"
#include <JANA/JLogger.h>

MetadataAggregator::MetadataAggregator() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}

void MetadataAggregator::Init() {

    auto app = GetApplication();

    // Allow the user to choose at runtime where we get our tracks
    app->SetDefaultParameter("metadata_example:track_factory", m_track_factory, "Options are 'generated' or 'smeared'");
    LOG << "MetadataAggregator::Init" << LOG_END;
}

void MetadataAggregator::Process(const std::shared_ptr<const JEvent> &event) {
    LOG << "MetadataAggregator::Process, Run #" << event->GetRunNumber() << ", Event #" << event->GetEventNumber() << LOG_END;

    // Acquire tracks in parallel
    auto tracks = event->Get<Track>(m_track_factory);

    // Lock mutex, so we can update shared state sequentially
    std::lock_guard<std::mutex>lock(m_mutex);

    // Since the run number probably doesn't change too frequently we cache the last entry
    int run_nr = event->GetRunNumber();
    if (run_nr != m_last_run_nr) {
        m_last_run_nr = run_nr;
        m_last_statistics = &m_statistics[m_last_run_nr]; // Get-or-create
    }

    // Update the statistics accumulator using the metadata from this event
    m_last_statistics->event_count += 1;
    m_last_statistics->total_track_count += tracks.size();
    m_last_statistics->total_latency_ns += event->GetMetadata<Track>(m_track_factory).elapsed_time_ns;
}

void MetadataAggregator::Finish() {
    LOG << "MetadataAggregator::Finish" << LOG_END;
    LOG << "Track factory tag = " << m_track_factory << LOG_END;

    // Print statistics organized by run number
    for (const auto& pair : m_statistics) {
        const int& run_nr = pair.first;
        const Statistics& statistics = pair.second;

        auto avg_track_count = statistics.total_track_count / statistics.event_count;
        auto avg_latency = statistics.total_latency_ns.count() / statistics.event_count;

        LOG << "run_nr=" << run_nr << " => avg track count= " << avg_track_count
            << ", latency= " << avg_latency << LOG_END;
    }
}

