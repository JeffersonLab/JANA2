
#ifndef _JFACTORYGENERATOR_TOYDET_H_
#define _JFACTORYGENERATOR_TOYDET_H_

#include <JANA/JFactoryGenerator.h>
#include <JANA/JFactoryT.h>

#include "rawSamples.h"

class JFactoryGenerator_toyDet:public JFactoryGenerator{

 public:

  void GenerateFactories(JFactorySet *factory_set){
		
    factory_set->Add(new JFactoryT<rawSamples>());

  }
};

#endif   // _JFACTORYGENERATOR_TOYDET_H_
