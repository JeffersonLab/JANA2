
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _SimpleClusterFactory_h_
#define _SimpleClusterFactory_h_

#include <JANA/JFactoryT.h>

#include "Cluster.h"

class SimpleClusterFactory : public JFactoryT<Cluster> {

    // Insert any member variables here

public:
    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;

};

#endif // _SimpleClusterFactory_h_
