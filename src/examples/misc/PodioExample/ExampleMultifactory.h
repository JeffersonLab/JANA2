
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_EXAMPLEMULTIFACTORY_H
#define JANA2_EXAMPLEMULTIFACTORY_H

#include <JANA/JMultifactory.h>

class ExampleMultifactory : public JMultifactory {
public:
    ExampleMultifactory();
    void Process(const std::shared_ptr<const JEvent>&) override;
};


#endif //JANA2_EXAMPLEMULTIFACTORY_H
