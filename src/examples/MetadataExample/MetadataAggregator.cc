
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
    LOG << "MetadataAggregator::Process, Event #" << event->GetEventNumber() << LOG_END;

    // Acquire tracks in parallel
    auto tracks = event->Get<Track>(m_track_factory);

    // Lock mutex, so we can update shared state sequentially
    std::lock_guard<std::mutex>lock(m_mutex);

    // Since the run number probably doesn't change too frequently we cache the last entry
    int run_number = event->GetRunNumber();
    if (run_number != last_run_nr) {
        last_run_nr = run_number;
        last_run_statistics = &track_factory_statistics[last_run_nr]; // Get-or-create
    }

    last_run_statistics->first += 1;  // event count
    last_run_statistics->second += event->GetMetadata<Track>(m_track_factory).elapsed_time_ns;
}

void MetadataAggregator::Finish() {
    LOG << "MetadataAggregator::Finish" << LOG_END;
    LOG << "Track factory tag = " << m_track_factory << LOG_END;

    // Print statistics organized by run number
    for (auto pair : track_factory_statistics) {
        auto avg_latency = pair.second.second.count()/pair.second.first;
        LOG << "run_nr: " << pair.first << " => latency: " << avg_latency << LOG_END;
    }
}

