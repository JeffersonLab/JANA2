
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTPROCESSORPODIO_H
#define JANA2_JEVENTPROCESSORPODIO_H

#include <JANA/JEventProcessor.h>
#include <podio/ROOTFrameWriter.h>

class JEventProcessorPodio : public JEventProcessor {

    Parameter<std::string> m_output_filename {this, "podio:output_filename", "podio_output.root", "Output filename for JEventProcessorPodio"};

    std::set<std::string> m_output_include_collections;
    std::set<std::string> m_output_exclude_collections;
    std::unique_ptr<podio::ROOTFrameWriter> m_writer;

public:
    JEventProcessorPodio();
    void Init() override;
    void Process(const JEvent&) override;
    void Finish() override;

};


#endif //JANA2_JEVENTPROCESSORPODIO_H
