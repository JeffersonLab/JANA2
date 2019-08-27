
#ifndef _JFactoryGenerator_toyDet_h_
#define _JFactoryGenerator_toyDet_h_

#include <JANA/JFactoryGenerator.h>
#include <JANA/JFactoryT.h>

#include "ADCSample.h"

class JFactoryGenerator_toyDet:public JFactoryGenerator{

 public:

  void GenerateFactories(JFactorySet *factory_set){
		
    factory_set->Add(new JFactoryT<ADCSample>());

  }

};

#endif // _JFactoryGenerator_toyDet_h_
