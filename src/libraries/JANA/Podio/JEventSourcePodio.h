
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTSOURCEPODIO_H
#define JANA2_JEVENTSOURCEPODIO_H

#include <JANA/JEventSource.h>
#include <podio/ROOTFrameReader.h>

template <template <typename> typename VisitT>
class JEventSourcePodio : public JEventSource {

protected:
    podio::ROOTFrameReader m_reader;
    uint64_t m_entry_count = 0;

public:
    // Constructor that is compatible with JEventSourceGenerator
    explicit JEventSourcePodio(std::string filename);


    /// User should NOT override GetEvent, because the way that we store
    /// the PODIO frame is part of the contract between JANA and PODIO. We
    /// don't prevent users from accessing the PODIO frame directly, but
    /// strongly discourage them from using this for anything other than debugging.
    Result Emit(JEvent&) final;


    /// User overrides NextFrame so that they can populate the frame however
    /// they like. This lets us support features such as:
    /// - datamodel glue (generated podio helper methods)
    /// - background events
    /// Event index is like event number except it starts at zero and increments.
    /// It is equivalent to Podio's record index.
    virtual std::unique_ptr<podio::Frame> NextFrame(int event_index, int& event_number, int& run_number) = 0;

    /// User may override Open() in case they need multiple files open concurrently,
    /// e.g. for background events. The existing implementation assumes exactly
    /// one file, in ROOT format.
    void Open() override;

    /// User may override Close() in case they need multiple files open concurrently,
    /// e.g. for background events. The existing implementation assumes exactly
    /// one file, in ROOT format.
    void Close() override;

};


template <typename T>
struct PodioCollectionMap;


struct InsertingVisitor {
    JEvent& m_event;
    const std::string& m_collection_name;

    InsertingVisitor(JEvent& event, const std::string& collection_name) : m_event(event), m_collection_name(collection_name){};

    template <typename T>
    void operator() (const T& collection) {
        using ContentsT = decltype(collection[0]);
        m_event.InsertCollectionAlreadyInFrame<ContentsT>(&collection, m_collection_name);
    }
};


template <template <typename> typename VisitT>
JEventSourcePodio<VisitT>::JEventSourcePodio(std::string filename)
        : JEventSource(filename) {
    SetCallbackStyle(CallbackStyle::ExpertMode);
}


template <template <typename> typename VisitT>
JEventSource::Result JEventSourcePodio<VisitT>::Emit(JEvent& event) {

    uint64_t event_index = event.GetEventNumber(); // Event number starts from zero by default
    if (event_index >= m_entry_count) return Result::FailureFinished;
    int event_number = 0;
    int run_number = 0;
    auto frame = NextFrame(event_index, event_number, run_number);
    event.SetEventNumber(event_number);
    event.SetRunNumber(run_number);

    VisitT<InsertingVisitor> visit;   // This is how we work on PODIO types while using collections
    for (const std::string& coll_name : frame->getAvailableCollections()) {
        const podio::CollectionBase* collection = frame->get(coll_name);
        InsertingVisitor visitor(event, coll_name);
        visit(visitor, *collection);
    }
    event.Insert(frame.release()); // Transfer ownership from unique_ptr to JFactoryT<podio::Frame>
    return Result::Success;
}


template <template <typename> typename VisitT>
void JEventSourcePodio<VisitT>::Open() {
    m_reader.openFile(GetResourceName());
    m_entry_count = m_reader.getEntries("events");
}

template <template <typename> typename VisitT>
void JEventSourcePodio<VisitT>::Close() {
    // TODO: Close ROOT file
}



#endif //JANA2_JEVENTSOURCEPODIO_H
