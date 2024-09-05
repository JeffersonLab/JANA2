// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: Nathan Brei

#include <JANA/JEventProcessor.h>
#include <podio/ROOTFrameWriter.h>

class PodioFileWriter : public JEventProcessor {

private:
    Parameter<std::vector<std::string>> m_collection_names {this, 
        "podio:output_collections", 
        {}, 
        "Comma-separated collection names to write to file"};

    Parameter<std::string> m_output_filename {this, 
        "podio:output_file", 
        "output.root", 
        "PODIO output filename"};

    Parameter<std::string> m_output_category {this, 
        "podio:output_category",
        "events",
        "Name of branch to store data in the output file"};

    std::unique_ptr<podio::ROOTFrameWriter> m_writer;


public:
    PodioFileWriter() {
        SetTypeName(NAME_OF_THIS);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Init() override {
        m_writer = std::make_unique<podio::ROOTFrameWriter>(*m_output_filename);
    }

    void Finish() override {
        m_writer->finish();
    }

    void ProcessParallel(const JEvent& event) override {

        for (const auto& collection_name : *m_collection_names) {

            // Trigger construction of everything specified in m_output_collections, in case it hadn't yet
            // Note that the frame might contain additional collections not specified in m_collection_names.
            // We are always writing these out as well for the sake of data integrity. If we don't, the file
            // may contain dangling references and segfault upon reading.
            event.GetCollectionBase(collection_name);
        }
    }

    void Process(const JEvent& event) override {

        auto* frame = event.GetSingle<podio::Frame>();
        // This will throw if no PODIO frame is found. 
        // As long as _some_ PODIO data has been inserted somewhere upstream, the frame will be present.

        m_writer->writeFrame(*frame, *m_output_category);
        // The user is responsible for setting the event/run numbers somewhere in their data model, so that
        // our output file can be correctly read. JANA doesn't/shouldn't know where that is!
        // The user should probably do this in either a JEventSource or a JEventUnfolder.
    }

};


extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->Add(new PodioFileWriter());
}
}



