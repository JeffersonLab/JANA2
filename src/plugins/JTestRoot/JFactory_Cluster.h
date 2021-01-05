// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JFactory_Cluster_h_
#define _JFactory_Cluster_h_

#include <JANA/JFactoryT.h>
#include "Cluster.h"

class JFactory_Cluster : public JFactoryT<Cluster> {

public:
    void Process(const std::shared_ptr<const JEvent> &event) override;

};

#endif // _JFactory_Cluster_h_
