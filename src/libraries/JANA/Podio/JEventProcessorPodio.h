
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTPROCESSORPODIO_H
#define JANA2_JEVENTPROCESSORPODIO_H

#include <JANA/JEventProcessor.h>
#include <podio/ROOTFrameWriter.h>

class JEventProcessorPodio : public JEventProcessor {

    std::string m_output_filename = "podio_output.root";
    std::set<std::string> m_output_include_collections;
    std::set<std::string> m_output_exclude_collections;
    std::unique_ptr<podio::ROOTFrameWriter> m_writer;

public:
    void Init() override;
    void Process(const std::shared_ptr<const JEvent>&) override;
    void Finish() override;

};


#endif //JANA2_JEVENTPROCESSORPODIO_H
