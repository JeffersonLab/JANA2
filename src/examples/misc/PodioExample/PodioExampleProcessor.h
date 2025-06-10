
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_PODIOEXAMPLEPROCESSOR_H
#define JANA2_PODIOEXAMPLEPROCESSOR_H

#include <JANA/JEventProcessor.h>

class PodioExampleProcessor : public JEventProcessor {
public:
    PodioExampleProcessor();
    void ProcessSequential(const JEvent&) override;
};


#endif //JANA2_PODIOEXAMPLEPROCESSOR_H
