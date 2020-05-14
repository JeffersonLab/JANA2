
#include "TrackSmearingFactory.h"
#include "TrackMetadata.h"

#include <JANA/JEvent.h>

void TrackSmearingFactory::Init() {
    auto app = GetApplication();
    
    /// Acquire any parameters
    // app->GetParameter("parameter_name", m_destination);
    
    /// Acquire any services
    // m_service = app->GetService<ServiceT>();
    
    /// Set any factory flags
    // SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);
}

void TrackSmearingFactory::ChangeRun(const std::shared_ptr<const JEvent> &event) {
    /// This is automatically run before Process, when a new run number is seen
    /// Usually we update our calibration constants by asking a JService
    /// to give us the latest data for this run number
    
    auto run_nr = event->GetRunNumber();
    // m_calibration = m_service->GetCalibrationsForRun(run_nr);
}

void TrackSmearingFactory::Process(const std::shared_ptr<const JEvent> &event) {

    auto raw_tracks = event->Get<Track>("generated");
    std::vector<Track*> results;

    // TODO: Start timer

    // TODO: Do some work
    // results.push_back(new Track(...));

    // TODO: Stop timer

    // Share results with TrackSmearingFactory
    Set(results);

    JMetadata<Track> metadata;
    metadata.elapsed_time_ns = std::chrono::nanoseconds {10};
    SetMetadata(metadata);
}
