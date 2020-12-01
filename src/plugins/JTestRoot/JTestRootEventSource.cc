// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
//
// This implements an event source to produce "fake" events. The events will
// contain only one type of object of class "Hit". The JFactory_Cluster
// object will then make Cluster objects from these later.
//
// Note that the "Hit" objects produced inherit from TObject but nothing
// special is needed here to accommodate that.

#include "JTestRootEventSource.h"
#include "Hit.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JPerfUtils.h>

JTestRootEventSource::JTestRootEventSource(std::string resource_name, JApplication* app) : JEventSource(resource_name, app) {
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
}

void JTestRootEventSource::GetEvent(std::shared_ptr <JEvent> event) {
    /// Generate an event by inserting objects into "event".
    /// (n.b. a normal event source would read these from a file or stream)

    // Spin the CPU a bit to limit the rate
    consume_cpu_ms(5);

    // Configure event and run numbers
    static size_t current_event_number = 1;
    event->SetEventNumber(current_event_number++);
    event->SetRunNumber(222);

    // Generate hit objects. We use random numbers to give some variation
    // and make things look a little more realistic
    std::default_random_engine generator;
    std::uniform_int_distribution<int> Hits_dist(3,10);
    std::uniform_int_distribution<int> Chan_dist(3,10);
    std::normal_distribution<double>   E_dist(0.75, 0.3);
    std::vector<Hit*> hits;
    auto Nhits = Hits_dist(generator);
    for( int i=0; i< Nhits; i++ ) {
        hits.push_back(new Hit(Chan_dist(generator), Chan_dist(generator), E_dist(generator)));
    }

    // Add Hit objects to event
    event->Insert(hits);
}

//template <>
//double JEventSourceGeneratorT<JTestRootEventSource>::CheckOpenable(std::string resource_name) {
//    return (resource_name == "JTestRootEventSource") ? 1.0 : 0.0;
//}
