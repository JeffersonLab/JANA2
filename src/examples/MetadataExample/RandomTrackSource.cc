
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "RandomTrackSource.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>

#include "Track.h"
#include "TrackMetadata.h"

/// There are two different ways of instantiating JEventSources
/// 1. Creating them manually and registering them with the JApplication
/// 2. Creating a corresponding JEventSourceGenerator and registering that instead
///    If you have a list of files as command line args, JANA will use the JEventSourceGenerator
///    to find the most appropriate JEventSource corresponding to that filename, instantiate and register it.
///    For this to work, the JEventSource constructor has to have the following constructor arguments:

RandomTrackSource::RandomTrackSource(std::string resource_name, JApplication* app) : JEventSource(resource_name, app) {
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

void RandomTrackSource::Open() {

    /// Open is called exactly once when processing begins.

    /// Get any configuration parameters from the JApplication
    // GetApplication()->SetDefaultParameter("RandomTrackSource:random_seed", m_seed, "Random seed");

    /// For opening a file, get the filename via:
    // std::string resource_name = GetResourceName();
    /// Open the file here!

    m_current_event_number = 0;
    m_current_run_number = 1;
    m_events_in_run = 5;
    m_max_run_number = 10;
}

RandomTrackSource::Result RandomTrackSource::Emit(JEvent& event) {

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.

    m_current_event_number += 1;
    if (m_current_event_number > m_events_in_run) {
        m_current_run_number += 1;
        m_current_event_number = 1;
    }

    if (m_current_run_number > m_max_run_number) {
        return Result::FailureFinished;
    }

    event.SetEventNumber(m_current_event_number);
    event.SetRunNumber(m_current_run_number);

    /// Insert whatever data was read into the event
    std::vector<Track*> tracks;
    tracks.push_back(new Track(0,0,0,0,0,0,0,0));
    event.Insert(tracks, "generated");

    // Insert metadata corresponding to these tracks
    // TODO: Overload event->Insert() to make this look nicer
    JMetadata<Track> metadata;
    metadata.elapsed_time_ns = std::chrono::nanoseconds {5};
    event.GetFactory<Track>("generated")->SetMetadata(metadata);

    return Result::Success;
}

std::string RandomTrackSource::GetDescription() {

    /// GetDescription() helps JANA explain to the user what is going on
    return "";
}


template <>
double JEventSourceGeneratorT<RandomTrackSource>::CheckOpenable(std::string resource_name) {

    /// CheckOpenable() decides how confident we are that this EventSource can handle this resource.
    ///    0.0        -> 'Cannot handle'
    ///    (0.0, 1.0] -> 'Can handle, with this confidence level'
    
    /// To determine confidence level, feel free to open up the file and check for magic bytes or metadata.
    /// Returning a confidence <- {0.0, 1.0} is perfectly OK!
    
    return (resource_name == "RandomTrackSource") ? 1.0 : 0.0;
}
