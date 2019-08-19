
#ifndef _JFACTORYGENERATOR_TOYDET_H_
#define _JFACTORYGENERATOR_TOYDET_H_

#include <JANA/JFactoryGenerator.h>
#include <JANA/JFactoryT.h>

#include "FECSample.h"

class JFactoryGenerator_toyDet:public JFactoryGenerator{

 public:

  void GenerateFactories(JFactorySet *factory_set){
		
    factory_set->Add(new JFactoryT<FECSample>());

  }
};

#endif   // _JFACTORYGENERATOR_TOYDET_H_
