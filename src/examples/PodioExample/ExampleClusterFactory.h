
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_EXAMPLECLUSTERFACTORY_H
#define JANA2_EXAMPLECLUSTERFACTORY_H

#include <JANA/Podio/JFactoryPodioT.h>
#include "datamodel/ExampleCluster.h"
#include "DatamodelGlue.h"

class ExampleClusterFactory : JFactoryPodioT<ExampleCluster> {
    void Process(const std::shared_ptr<const JEvent> &ptr) override;
};


#endif //JANA2_EXAMPLECLUSTERFACTORY_H
