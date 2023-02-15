
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JEventSourcePodio.h"


struct InsertingVisitor {
    JEvent& m_event;
    const std::string& m_collection_name;

    InsertingVisitor(JEvent& event, const std::string& collection_name) : m_event(event), m_collection_name(collection_name){};

    template <typename T>
    void operator() (const typename PodioTypeMap<T>::collection_t& collection) {
        m_event.InsertCollection<T>(collection, m_collection_name);
    }
};

JEventSourcePodio::JEventSourcePodio(std::string filename)
: JEventSource(filename) {
}

void JEventSourcePodio::GetEvent(std::shared_ptr<JEvent> event) {
    int event_index = event->GetEventNumber(); // Event number starts from zero by default
    int event_number = 0;
    int run_number = 0;
    auto frame = NextFrame(event_index, event_number, run_number);
    event->SetEventNumber(event_number);
    event->SetRunNumber(run_number);
    event->Insert(frame.release());

    // event->InsertCollection<>()
    // TODO: Each collection in the frame needs to be inserted into a JFactory
}

void JEventSourcePodio::Open() {
    // TODO: Open ROOT file and fail if necessary
}

void JEventSourcePodio::Close() {
    // TODO: Close ROOT file
}


