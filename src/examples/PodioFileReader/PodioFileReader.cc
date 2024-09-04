
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

#include <PodioDatamodel/EventInfoCollection.h>
#include <PodioDatamodel/TimesliceInfoCollection.h>
#include <PodioDatamodel/ExampleHitCollection.h>
#include <PodioDatamodel/ExampleClusterCollection.h>

#include <podio/ROOTFrameReader.h>



class PodioFileReader : public JEventSource {

private:
    uint64_t m_entry_count = 0;
    podio::ROOTFrameReader m_reader;
    // ROOTFrameReader emits a lot of deprecation warnings, but the supposed replacement
    // won't actually be a replacement until the next version

public:
    PodioFileReader() {
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    static std::string GetDescription() {
        return "Example that reads a PODIO file into JANA. This example uses `PodioDatamodel`, but it is trivial to adapt it to any data model. For now this only works on files containing events, not timeslices or any other levels.";
    }

    void Open() final {
        m_reader.openFile(GetResourceName());
        m_entry_count = m_reader.getEntries("events");
    }

    void Close() final {
        // ROOT(Frame)Reader doesn't support closing the file (!). 
        // Maybe we can do this via ROOT. 
    }

    Result Emit(JEvent& event) final {

        // Figure out which entry to read

        uint64_t event_index = event.GetEventNumber(); // Event number starts from zero by default
        if (event_index >= m_entry_count) return Result::FailureFinished;

        // Read entry

        auto frame_data = m_reader.readEntry("events", event_index);
        auto frame = std::make_unique<podio::Frame>(std::move(frame_data));

        // Extract event key (event nr, run nr, timeslice nr, etc)

        auto& eventinfo = frame->get<EventInfoCollection>("eventinfos");
        if (eventinfo.size() != 1) throw JException("Bad eventinfo: Entry %d contains %d items, 1 expected.", event_index, eventinfo.size());
        event.SetEventNumber(eventinfo[0].EventNumber());
        event.SetRunNumber(eventinfo[0].RunNumber());

        // Make all PODIO collections available to JANA

        for (const std::string& coll_name : frame->getAvailableCollections()) {

            // It is possible to only put a subset of the collections into the JEvent.
            // However this is risky because PODIO tracks inter-object associations but
            // doesn't track the resulting inter-collection associations. Thus if the user
            // omits any collections when they read a file, they risk introducing dangling 
            // pointers. This can silently corrupt the data in the output file.

            const podio::CollectionBase* coll = frame->get(coll_name);
            const auto& coll_type = coll->getValueTypeName();

            if (coll_type == "EventInfo") {
                event.InsertCollectionAlreadyInFrame<EventInfo>(coll, coll_name);
            }
            else if (coll_type == "TimesliceInfo") {
                event.InsertCollectionAlreadyInFrame<TimesliceInfo>(coll, coll_name);
            }
            else if (coll_type == "ExampleHit") {
                event.InsertCollectionAlreadyInFrame<ExampleHit>(coll, coll_name);
            }
            else if (coll_type == "ExampleCluster") {
                event.InsertCollectionAlreadyInFrame<ExampleCluster>(coll, coll_name);
            }
            else {
                throw JException("Collection '%s' has typename '%s' which is not known to PodioFileReader", coll_name.c_str(), coll_type.data());
            }
        }

        // Put the frame in the events as well, tying their lifetimes together

        event.Insert(frame.release()); // Transfer ownership from unique_ptr to JFactoryT<podio::Frame>
        return Result::Success;
    }
};



extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->Add(new JEventSourceGeneratorT<PodioFileReader>);
}
}

