
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "RandomSource.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>

/// Include headers to any JObjects you wish to associate with each event
#include "Hit.h"

/// There are two different ways of instantiating JEventSources
/// 1. Creating them manually and registering them with the JApplication
/// 2. Creating a corresponding JEventSourceGenerator and registering that instead
///    If you have a list of files as command line args, JANA will use the JEventSourceGenerator
///    to find the most appropriate JEventSource corresponding to that filename, instantiate and register it.
///    For this to work, the JEventSource constructor has to have the following constructor arguments:

RandomSource::RandomSource() : JEventSource() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

RandomSource::RandomSource(std::string resource_name, JApplication* app) : JEventSource(resource_name, app) {
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

void RandomSource::Open() {

    /// Open is called exactly once when processing begins.
    
    /// Get any configuration parameters from the JApplication
    JApplication* app = GetApplication();
    app->SetDefaultParameter("random_source:max_emit_freq_hz",
                             m_max_emit_freq_hz,
                             "Maximum event rate [Hz] for RandomSource");

    /// For opening a file, get the filename via:
    // std::string resource_name = GetResourceName();
    /// Open the file here!
}

void RandomSource::Close() {
    // Close the file pointer here!
}

JEventSource::Result RandomSource::Emit(JEvent& event) {

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.
    
    /// Configure event and run numbers
    static size_t current_event_number = 1;
    event.SetEventNumber(current_event_number++);
    event.SetRunNumber(22);

    /// Slow down event source
    auto delay_ms = std::chrono::milliseconds(1000/m_max_emit_freq_hz);
    std::this_thread::sleep_for(delay_ms);

    /// Insert simulated data into event
    std::vector<Hit*> hits;
    hits.push_back(new Hit(0, 0, 1.0, 0));
    hits.push_back(new Hit(0, 1, 1.0, 0));
    hits.push_back(new Hit(1, 0, 1.0, 0));
    hits.push_back(new Hit(1, 1, 1.0, 0));
    event.Insert(hits);

    /// If you are reading a file of events and have reached the end
    /// Note that you should close the file handle in Close(), not here.
    // return Result::FailureFinished;

    /// If you are streaming events and there are no new events in the message queue,
    /// tell JANA that GetEvent() was temporarily unsuccessful like this:
    // return Result::FailureTryAgain;

    return Result::Success;
}

std::string RandomSource::GetDescription() {

    /// GetDescription() helps JANA explain to the user what is going on
    return "";
}


template <>
double JEventSourceGeneratorT<RandomSource>::CheckOpenable(std::string resource_name) {

    /// CheckOpenable() decides how confident we are that this EventSource can handle this resource.
    ///    0.0        -> 'Cannot handle'
    ///    (0.0, 1.0] -> 'Can handle, with this confidence level'
    
    /// To determine confidence level, feel free to open up the file and check for magic bytes or metadata.
    /// Returning a confidence <- {0.0, 1.0} is perfectly OK!
    
    return (resource_name == "random") ? 1.0 : 0.0;
}
