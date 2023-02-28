
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_PODIOEXAMPLEPROCESSOR_H
#define JANA2_PODIOEXAMPLEPROCESSOR_H

#include <JANA/JEventProcessor.h>
#include "DatamodelGlue.h"

class PodioExampleProcessor : public JEventProcessor {
public:
    void Process(const std::shared_ptr<const JEvent> &ptr) override;


};


#endif //JANA2_PODIOEXAMPLEPROCESSOR_H
