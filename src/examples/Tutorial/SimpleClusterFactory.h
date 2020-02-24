
#ifndef _SimpleClusterFactory_h_
#define _SimpleClusterFactory_h_

#include <JANA/JFactoryT.h>

#include "Cluster.h"

class SimpleClusterFactory : public JFactoryT<Cluster> {

    // Insert any member variables here

public:
    SimpleClusterFactory() : JFactoryT<Cluster>(NAME_OF_THIS) {};
    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;

};

#endif // _SimpleClusterFactory_h_
