#ifndef _JFactoryGenerator_jana_test_
#define _JFactoryGenerator_jana_test_

#include <JANA/JFactoryGenerator.h>
#include "JResourcePool.h"

#include "JTestDummyDataFactory.h"

class JTestDummyDataFactoryGenerator : public JFactoryGenerator {
public:

    virtual const char *className(void) {
        return static_className();
    }

    static const char *static_className(void) {
        return "JTestDummyDataFactoryGenerator";
    }

    void GenerateFactories(JFactorySet *factory_set) {
        factory_set->Add(new JTestDummyDataFactory());
    }
};

#endif // _JFactoryGenerator_jana_test_

