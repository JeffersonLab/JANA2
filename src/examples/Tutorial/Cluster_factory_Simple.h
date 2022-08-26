
#ifndef _Cluster_factory_Simple_h_
#define _Cluster_factory_Simple_h_

#include <JANA/JFactoryT.h>

#include "Cluster.h"

class Cluster_factory_Simple : public JFactoryT<Cluster> {

    // Insert any member variables here

public:
    Cluster_factory_Simple();
    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;

};

#endif // _Cluster_factory_Simple_h_
