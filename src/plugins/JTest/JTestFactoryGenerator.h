#ifndef JTestFactoryGenerator_h
#define JTestFactoryGenerator_h

#include <JANA/JFactoryGenerator.h>
#include "JResourcePool.h"
#include "JTestFactories.h"

class JTestFactoryGenerator : public JFactoryGenerator {
public:

    virtual const char *className(void) {
        return static_className();
    }

    static const char *static_className(void) {
        return "JTestFactoryGenerator";
    }

    void GenerateFactories(JFactorySet *factory_set) {
        factory_set->Add(new JTestDisentangler());
        factory_set->Add(new JTestTracker());
    }
};

#endif // JTestFactoryGenerator_h

