
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_PODIOEXAMPLESOURCE_H
#define JANA2_PODIOEXAMPLESOURCE_H

#include "DatamodelGlue.h"   // Has to come BEFORE JEventSourcePodio.h
#include <JANA/Podio/JEventSourcePodio.h>

class PodioExampleSource : public JEventSourcePodio<DatamodelCollectionVisit> {
public:
    explicit PodioExampleSource(std::string filename) : JEventSourcePodio<DatamodelCollectionVisit>(std::move(filename)) {
    }
    std::unique_ptr<podio::Frame> NextFrame(int event_index, int &event_number, int &run_number) override;

};


#endif //JANA2_PODIOEXAMPLESOURCE_H
