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
#include <JANA/Utils/JBenchUtils.h>

JTestRootEventSource::JTestRootEventSource() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
    SetCallbackStyle(CallbackStyle::ExpertMode);
}

JEventSource::Result JTestRootEventSource::Emit(JEvent& event) {
    /// Generate an event by inserting objects into "event".
    /// (n.b. a normal event source would read these from a file or stream)

    // Configure event and run numbers
    static size_t current_event_number = 1;
    event.SetEventNumber(current_event_number++);
    event.SetRunNumber(222);
    
    m_bench_utils.set_seed(event.GetEventNumber(), NAME_OF_THIS);
    // Spin the CPU a bit to limit the rate
    m_bench_utils.consume_cpu_ms(5);

    // Generate hit objects. We use random numbers to give some variation
    // and make things look a little more realistic
    std::uniform_int_distribution<int>    Hits_dist(3,10);
    std::uniform_int_distribution<int>    Chan_dist(-40,400);
    std::normal_distribution<double>      E_dist(0.75, 0.3);
    std::uniform_real_distribution<float> t_dist(30.0, 80.);
    std::vector<Hit*> hits;
    auto Nhits = Hits_dist(generator);
    for( int i=0; i< Nhits; i++ ) {
        hits.push_back(new Hit(Chan_dist(generator), Chan_dist(generator), E_dist(generator), t_dist(generator)));
    }

    // Add Hit objects to event
    event.Insert(hits);
    return Result::Success;
}

