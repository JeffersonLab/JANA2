
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _DstExampleFactory_h_
#define _DstExampleFactory_h_

#include <JANA/JFactoryT.h>

#include "DataObjects.h"

class DstExampleFactory : public JFactoryT<MyRenderableJObject> {

    // Insert any member variables here

public:
    DstExampleFactory() : JFactoryT<MyRenderableJObject>(NAME_OF_THIS, "from_factory") {};
    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;

};

#endif // _DstExampleFactory_h_
